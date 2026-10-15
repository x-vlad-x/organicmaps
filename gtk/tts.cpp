#include "tts.hpp"

#include "l10n.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"

#include <glib.h>

namespace gtk::tts
{
namespace
{
char const kEnabledKey[] = "GtkTtsEnabled";

struct Engine
{
  std::string spdSay;
  std::string espeak;
};

Engine const & GetEngine()
{
  static Engine const engine = []()
  {
    Engine e;
    if (char * p = g_find_program_in_path("spd-say"))
    {
      e.spdSay = p;
      g_free(p);
    }
    if (char * p = g_find_program_in_path("espeak-ng"))
    {
      e.espeak = p;
      g_free(p);
    }
    LOG(LINFO, ("TTS engine:", !e.spdSay.empty() ? e.spdSay : (e.espeak.empty() ? "none" : e.espeak)));
    return e;
  }();
  return engine;
}

// The language code understood by both speech-dispatcher and espeak-ng.
std::string Voice()
{
  auto lang = CurrentLanguage();
  if (auto const dash = lang.find('-'); dash != std::string::npos)
    lang = lang.substr(0, dash);
  return lang;
}
}  // namespace

bool IsAvailable()
{
  auto const & e = GetEngine();
  return !e.spdSay.empty() || !e.espeak.empty();
}

bool IsEnabled()
{
  bool enabled = true;
  settings::TryGet(kEnabledKey, enabled);
  return enabled && IsAvailable();
}

void SetEnabled(bool enabled)
{
  settings::Set(kEnabledKey, enabled);
}

void Speak(std::string const & phrase)
{
  if (phrase.empty() || !IsAvailable())
    return;
  auto const & e = GetEngine();
  std::vector<std::string> args;
  if (!e.spdSay.empty())
    args = {e.spdSay, "-l", Voice(), "-r", "10", phrase};
  else
    args = {e.espeak, "-v", Voice(), "-s", "165", phrase};

  std::vector<char *> argv;
  for (auto & a : args)
    argv.push_back(a.data());
  argv.push_back(nullptr);

  GError * error = nullptr;
  if (!g_spawn_async(nullptr, argv.data(), nullptr, G_SPAWN_DEFAULT, nullptr, nullptr, nullptr, &error))
  {
    LOG(LWARNING, ("TTS failed:", error->message));
    g_clear_error(&error);
  }
}

void Speak(std::vector<std::string> const & phrases)
{
  std::string text;
  for (auto const & p : phrases)
  {
    if (!text.empty())
      text += ". ";
    text += p;
  }
  Speak(text);
}
}  // namespace gtk::tts
