#include "engine/audio/audio_data.h"
#include "engine/audio/audio_manager.h"
#include "engine/config/config.h"
#include "engine/resources/resources.h"
#include "test_support.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace rage
{
namespace
{
using test::Check;
using test::ClickAction;
using test::Near;
using test::TestDirectory;

bool HasProblem(const AudioData& data, const std::string& fragment)
{
    return std::any_of(data.Problems().begin(), data.Problems().end(),
                       [&](const std::string& problem) { return problem.find(fragment) != std::string::npos; });
}

bool Contains(const std::vector<std::string>& list, const std::string& value)
{
    return std::find(list.begin(), list.end(), value) != list.end();
}

const char* sampleAudioCsv =
    "Sound ID,File Name,Category,Volume,Loops,Variants,Notes\r\n"
    "ui_click,click.wav,UI,0.8,,,\r\n"
    "player_jump,jump.wav,SFX,1,,3,\"Three takes, picked at random\"\r\n"
    "level_1_theme,level_1.ogg,Music,0.7,,,\r\n"
    "narrator_hello,,Voice,,No,,no file yet\r\n"
    ",,,,,,\r\n";

void TestAudioImport()
{
    AudioData data;
    Check(data.Parse(sampleAudioCsv, "sample") && data.Size() == 4, "Audio Data did not import every valid row");
    const auto* click = data.Find("ui_click");
    const auto* jump = data.Find("player_jump");
    const auto* theme = data.Find("level_1_theme");
    const auto* hello = data.Find("narrator_hello");
    Check(click && jump && theme && hello, "Audio lookups failed");
    Check(click->category == AudioCategory::Ui && Near(click->volume, 0.8f, 0.001f) && !click->loops,
          "UI row was not parsed");
    Check(jump->variants == 3 && jump->notes == "Three takes, picked at random", "Variants or quoted notes were not parsed");
    Check(theme->IsMusic() && theme->loops && Near(theme->volume, 0.7f, 0.001f), "Music should loop by default");
    Check(hello->category == AudioCategory::Voice && hello->fileName.empty() && hello->volume == 1,
          "A planned sound without a file should import with defaults");

    Check(VariantFileNames(*jump) == std::vector<std::string>{"jump_1.wav", "jump_2.wav", "jump_3.wav"},
          "Variant file names are wrong");
    Check(VariantFileNames(*click) == std::vector<std::string>{"click.wav"} && VariantFileNames(*hello).empty(),
          "Single or missing file names are wrong");
    SoundData folder;
    folder.fileName = "my.folder/rain";
    folder.variants = 2;
    Check(VariantFileNames(folder) == std::vector<std::string>{"my.folder/rain_1", "my.folder/rain_2"},
          "A dot in a folder name was treated as an extension");

    Check(Near(EffectiveVolume(*click, 0.5f), 0.4f, 0.001f) && EffectiveVolume(*jump, 2.0f) == 1,
          "Row and category volumes did not combine and clamp");
}

void TestAudioValidation()
{
    AudioData data;
    const bool clean = data.Parse(
        "Sound ID,File Name,Category,Volume,Loops,Variants,Notes\n"
        "good,good.wav,SFX,0.5,,,\n"
        "good,other.wav,SFX,,,,\n"
        ",nameless.wav,SFX,,,,\n"
        "loud,loud.wav,Explosion,loud,maybe,0,\n"
        "too_loud,too_loud.wav,SFX,1.5,,x,\n"
        "theme,theme.ogg,Music,,,2,\n"
        "Bad-Name,bad.wav,SFX,,,,\n",
        "invalid");
    Check(!clean && data.Size() == 5, "Invalid rows were not reported, or valid rows were lost");
    Check(data.Find("good")->fileName == "good.wav", "A duplicate replaced the first row");
    const auto* loud = data.Find("loud");
    Check(loud->category == AudioCategory::Sfx && loud->volume == 1 && !loud->loops && loud->variants == 1,
          "Invalid values did not fall back to defaults");
    Check(data.Find("too_loud")->volume == 1, "An out-of-range volume was not clamped");
    Check(data.Find("theme")->variants == 1, "Music variants were not ignored");
    for (const char* fragment : {"duplicate", "Sound ID is blank", "Category", "Volume", "Loops", "Variants",
                                 "music cannot have variants", "lowercase"})
        Check(HasProblem(data, fragment), "An invalid audio value was not reported");

    AudioData renamed;
    renamed.Parse("Sound ID,File Name,Kind,Volume,Loops,Variants,Notes\nbeep,beep.wav,UI,,,,\n");
    Check(renamed.Find("beep")->category == AudioCategory::Ui && HasProblem(renamed, "column C"),
          "A renamed header was not read by position");
    AudioData noIds;
    // Column A holds another known header, so Sound ID can't be guessed by position.
    Check(!noIds.Parse("File Name,Category\nbeep.wav,SFX\n") && noIds.Size() == 0, "Missing Sound ID column should import nothing");
    AudioData missing;
    Check(!missing.LoadFile("definitely/not/here/audio_data.csv") && missing.Size() == 0 && !missing.Problems().empty(),
          "A missing Audio Data file should be reported, not thrown");
}

void TestAudioSettings()
{
    TestDirectory directory;
    const auto file = directory.path / "audio.yaml";
    {
        std::ofstream output(file);
        output << "audio:\n  master_volume: 0.6\n  muted: false\n  music_volume: 0.3\n  sfx_volume: 0.9\n";
    }
    ConfigStore store;
    Check(store.values.musicVolume == 0.8f && store.values.sfxVolume == 1.0f, "Default music/SFX volumes are wrong");
    Check(store.Load(file) && Near(store.values.musicVolume, 0.3f, 0.001f) && Near(store.values.sfxVolume, 0.9f, 0.001f),
          "Music/SFX volumes were not loaded");
    const auto saved = directory.path / "saved.yaml";
    Check(store.Save(saved), "Settings save failed");
    ConfigStore reloaded;
    Check(reloaded.Load(saved) && Near(reloaded.values.musicVolume, 0.3f, 0.001f) && Near(reloaded.values.sfxVolume, 0.9f, 0.001f),
          "Music/SFX volumes did not survive a save");
}

// A level with no stage behaviour, so the test depends only on Level 0's layout.
class SoundTestLevel final : public Level
{
public:
    explicit SoundTestLevel(const LevelDefinition& definition) : Level(definition) {}
    using Level::MutablePlayer;
    using Level::RequestSound;
};

void TestSoundRequests()
{
    GameConfig config;
    Game game(config);
    Check(game.MusicTrack() == menuMusic && game.TakeSoundRequests().empty() && !game.IsLevelPaused(),
          "Menus should play the menu theme and request no sounds");
    game.Update(0, ClickAction(game, ActionKind::StartLevel, static_cast<int>(LevelId::Level0)));
    Check(game.MusicTrack() == "level_0_theme" && !game.IsLevelPaused(), "A level should play its own track");
    game.Update(0, {.pause = true});
    Check(game.IsLevelPaused() && game.MusicTrack() == "level_0_theme", "Pause should keep the level's (ducked) music");

    const LevelDefinition& definition = Level0Definition();
    SoundTestLevel level(definition);
    level.Reset(config);
    level.Update(1.0f / 60, {}, config);
    Check(level.TakeSoundRequests().empty(), "Appearing at the spawn point requested a sound");
    for (int i = 0; i < 60; ++i) level.Update(1.0f / 60, {}, config);
    level.TakeSoundRequests();
    Check(level.GetPlayer().Grounded(), "Level 0's spawn should rest on the floor");

    level.Update(1.0f / 60, {.jump = true}, config);
    Check(Contains(level.TakeSoundRequests(), playerSounds::jump), "Jumping did not request the jump sound");
    bool landed = false;
    for (int i = 0; i < 240 && !landed; ++i)
    {
        level.Update(1.0f / 60, {}, config);
        landed = Contains(level.TakeSoundRequests(), playerSounds::land);
    }
    Check(landed, "Landing did not request the land sound");
    Check(level.TakeSoundRequests().empty(), "Sound requests were not cleared after being taken");

    // Being put back at the spawn mid-jump (a respawn or script) is not a landing.
    level.Update(1.0f / 60, {.jump = true}, config);
    for (int i = 0; i < 10; ++i) level.Update(1.0f / 60, {}, config);
    level.TakeSoundRequests();
    level.MutablePlayer().Reset(definition.spawn);
    for (int i = 0; i < 5; ++i) level.Update(1.0f / 60, {}, config);
    Check(!Contains(level.TakeSoundRequests(), playerSounds::land), "A teleport to the spawn was reported as a landing");

    level.RequestSound("door_creak");
    level.RequestSound("");
    Check(level.TakeSoundRequests() == std::vector<std::string>{"door_creak"}, "Scripted sound requests failed");
    Check(level.MusicTrack() == "level_0_theme", "Default level music ID is wrong");
}
}

void TestAudioData()
{
    TestAudioImport();
    TestAudioValidation();
    TestAudioSettings();
    TestSoundRequests();
    std::cout << "PASS: Audio Data import/validation, variants, volumes, settings and level sound requests\n";
}

// Needs a raylib window and an audio device; skipped cleanly without speakers.
void TestAudioPlayback()
{
    if (!IsAudioDeviceReady())
    {
        std::cout << "SKIP: audio playback (no audio device)\n";
        return;
    }
    TestDirectory fixture;
    std::filesystem::create_directories((fixture.path / audioDataPath).parent_path());
    std::filesystem::create_directories(fixture.path / sfxFolder);
    std::filesystem::create_directories(fixture.path / musicFolder);
    {
        std::ofstream csv(fixture.path / audioDataPath, std::ios::binary);
        csv << "Sound ID,File Name,Category,Volume,Loops,Variants,Notes\r\n"
               "beep,beep.wav,SFX,1,,,\r\n"
               "multi,multi.wav,SFX,1,,2,\r\n"
               "theme,loop.wav,Music,1,Yes,,\r\n"
               "silent,,SFX,,,,\r\n"
               "missing,nope.wav,SFX,,,,\r\n";
    }
    // Generated test tones: a 440 Hz sine wave, 16-bit mono.
    const auto writeTone = [&](const std::filesystem::path& path, float seconds) {
        const unsigned int sampleRate = 22050;
        std::vector<short> samples(static_cast<std::size_t>(sampleRate * seconds));
        for (std::size_t i = 0; i < samples.size(); ++i)
            samples[i] = static_cast<short>(8000 * std::sin(2 * 3.14159265 * 440 * static_cast<double>(i) / sampleRate));
        Wave wave{static_cast<unsigned int>(samples.size()), sampleRate, 16, 1, samples.data()};
        return ExportWave(wave, path.string().c_str());
    };
    Check(writeTone(fixture.path / sfxFolder / "beep.wav", 0.5f) && writeTone(fixture.path / sfxFolder / "multi_1.wav", 0.5f) &&
          writeTone(fixture.path / sfxFolder / "multi_2.wav", 0.5f) && writeTone(fixture.path / musicFolder / "loop.wav", 1.0f),
          "Could not create audio fixtures");

    Resources resources(fixture.path);
    auto& audio = resources.Audio();
    Check(audio.Data().Size() == 5 && audio.Data().Problems().empty(), "Fixture Audio Data did not load cleanly");
    Check(audio.IsLoaded("beep") && audio.IsLoaded("multi") && !audio.IsLoaded("silent") && !audio.IsLoaded("missing"),
          "Audio files were not loaded, or missing files were not left silent");

    audio.Play("beep");
    Check(audio.IsPlaying("beep"), "A sound effect did not play");
    for (int i = 0; i < AudioManager::voicesPerSound * 2; ++i) audio.Play("beep"); // More than the voices: must not fail.
    audio.Play("multi");
    Check(audio.IsPlaying("multi"), "A sound with variants did not play");
    audio.Play("silent");
    audio.Play("missing");
    audio.Play("no_such_sound"); // All three stay silent without crashing.

    audio.PlayMusic("theme", 0);
    audio.Update(1.0f / 60);
    Check(audio.CurrentMusic() == "theme" && audio.IsMusicPlaying(), "Music did not start");
    audio.PlayMusic("theme", 0); // Same track: must keep playing, not restart.
    Check(audio.IsMusicPlaying(), "Asking for the current track stopped it");

    audio.SetPaused(true);
    Check(!audio.IsPlaying("beep") && !audio.IsPlaying("multi") && audio.IsMusicPlaying(),
          "Pausing should stop sound effects but keep (ducked) music");
    audio.SetPaused(false);

    audio.StopMusic(0);
    audio.Update(1.0f / 60);
    Check(audio.CurrentMusic().empty() && !audio.IsMusicPlaying(), "Music did not stop");
    std::cout << "PASS: audio playback (sound effects, voices, variants, music, pause and missing files)\n";
}
}
