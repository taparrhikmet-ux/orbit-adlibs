# Orbit Adlibs — source project / version 0.1.0

An original, adlib-focused vocal effects plug-in inspired by spacious, modulated,
saturated psychedelic rap vocal production. Not affiliated with or endorsed by
Travis Scott. It does not claim to reproduce his exact recording chain.

IMPORTANT: THIS PACKAGE CONTAINS SOURCE CODE, NOT A COMPILED PLUG-IN.
The code has not been compiled, host-validated, or auditioned on macOS here.
Treat this as an untested development prototype, not a production-ready release.
The build configuration targets macOS 12.0 and newer; that is not proof of runtime
compatibility. Actual macOS 12 and DAW testing is still required.

## What you get

- VST3 and Audio Unit v2 build targets. No legacy VST2.
- A custom 840 x 590 dark interface with purple and cyan controls.
- Chorus, Saturation, and Reverb as the three large main controls.
- Tempo-synced ping-pong delay (1/8, dotted 1/8, or 1/4).
- Stereo width, dark/bright tone, reverb space, overall mix, and output trim.
- Four starting presets: Clean Orbit, Wide Haze, Dark Grit, Space Throw.
- Parameter automation and host session state recall through JUCE APVTS.
- Mono, stereo, and mono-in/stereo-out layouts, where supported by the host.
- Smoothed main continuous controls and no intentional reported plug-in latency.
- No internet requirement for the plug-in itself. Internet is needed to fetch JUCE
  during the first build unless you supply a local JUCE folder.

## Requirements

1. A Mac running macOS 12 or later.
2. Apple Command Line Tools, including a compatible macOS SDK and C++ compiler.
   Install from Terminal using `xcode-select --install` if needed.
   Full Xcode is not required by the supplied Makefiles build.
3. CMake 3.22 or newer available in your shell PATH, plus Git.
   Use the official CMake macOS installer or, if Homebrew is already installed,
   `brew install cmake`. A CMake app installation may require adding
   `/Applications/CMake.app/Contents/bin` to your PATH.
   Select installer/tool versions that support your macOS version.
4. Internet access for the first build to download JUCE 7.0.12.
5. A VST3 or Audio Unit host that itself supports your macOS version.

No JUCE Projucer project is required. JUCE is fetched at the pinned 7.0.12 tag.
Alternatively, put a complete compatible JUCE source checkout in a folder named
JUCE beside CMakeLists.txt for an offline dependency source.

## Build on your Mac

1. Extract the ZIP to a normal writable folder.
2. Run build.command for your Mac's native architecture.
   If Finder does not allow the script to open, open Terminal, type `cd `,
   drag the extracted folder into Terminal, press Return, then run:

   bash build.command

3. For a combined Intel + Apple Silicon binary, run instead:

   bash build-universal.command

4. Wait for compilation. The two resulting bundles are in `dist`:
   - Orbit Adlibs.vst3
   - Orbit Adlibs.component
5. Close your DAW, run `bash install.command`, and approve the install prompt.
   Existing installed copies are backed up to your user Application Support folder.
6. Run `bash validate-au.command` to exercise Apple's AU validator.
7. Reopen your DAW and rescan plug-ins if necessary.

Installation locations:
- VST3: ~/Library/Audio/Plug-Ins/VST3/
- Audio Unit: ~/Library/Audio/Plug-Ins/Components/

Logic Pro uses the Audio Unit version. In hosts supporting VST3, use the VST3.
On Apple Silicon, use a native arm64 build with a native host, or use a universal
build to cover both native and Rosetta hosts. An Intel-only host needs x86_64 code.
The build script uses local ad-hoc signing, not Developer ID signing/notarization.
For third-party distribution, perform proper signing/notarization and validation;
do not disable Gatekeeper globally to install this prototype.

To remove the installed plug-in, close the DAW and remove only the two Orbit
Adlibs bundles from the locations above, then rescan the host.

## Controls

| Control | Meaning |
|---|---|
| Chorus | Blend of short stereo modulated delays for moving doubles |
| Saturation | Amount of soft nonlinear drive, with approximate gain compensation |
| Reverb | Parallel reverb return level; it does not replace the insert's dry vocal |
| Delay | Level of a filtered feedback ping-pong echo |
| Width | 0 = mono, 50 = neutral stereo width, 100 = double side gain |
| Tone | Low-pass cutoff from about 2.2 kHz to 17.6 kHz; not a high-shelf boost |
| Space | Reverb room-size setting, not a calibrated RT60 in seconds |
| Mix | Original input vs the entire processed vocal chain |
| Output | -24 to +6 dB final trim; starts at -3 dB |
| Echo Sync | Host-tempo subdivision; defaults to 120 BPM if no tempo is available |

The tone cutoff is capped below Nyquist at lower sample rates. Width cannot make
a mono output bus stereo. The fixed 140 Hz high-pass is part of the wet/processed
path only. The original input remains available through Mix.

## Signal path

Original input -------------------------------> overall dry/wet blend
Input -> 140 Hz high-pass -> saturation -> tone -> stereo chorus -> echo
                                                        -> parallel reverb
        -> stereo mid/side width -> overall dry/wet blend -> output -> safety clamp

The main wet path retains a direct vocal component. This version is an INSERT
processor, not a true wet-only auxiliary-send effect. Mix = 100% is the complete
processed vocal, not isolated echo/reverb. Put it directly on an adlib track or
adlib bus. For a dedicated reverb/delay send, use a wet-only processor instead.

Mix = 0% still passes through Output and the emergency sample clamp, so it is not
bit-perfect bypass. Use the DAW's bypass to completely bypass the effect.
Echo/reverb continue running at zero return level, so increasing their knobs can
reveal existing tails. Bypassed behavior is controlled by the host.

## Adlib starting points

Start with Clean Orbit, then:
- Wide Haze: more moving doubles and diffuse space.
- Dark Grit: darker, more saturated and closer adlibs.
- Space Throw: longer ambience and stronger quarter-note echo.

Automate the Reverb and Delay levels on the final words of phrases for throws.
These are design starting points, not auditioned or loudness-matched presets.

## Safety and limitations

- First test at low monitor/headphone volume in an empty session.
- Lower Output if the red ceiling indicator flashes. The final clamp at +/-0.98
  is emergency sample clipping, NOT a transparent limiter or true-peak limiter.
- Saturation is not oversampled, so aggressive settings can alias. This is a
  deliberate prototype simplification, not an emulation of analog circuitry.
- There is no pitch correction, formant shifting, dedicated compressor, vocal
  detector, intelligent ducking, or automatic matching of another artist's voice.
- Reverb is JUCE's algorithmic reverb, not a convolution IR or measured room.
- Echo timing is clamped to 30 ms–2 seconds. Very slow tempos may hit that limit.
  Tempo or subdivision changes slide the delay read position and may pitch-bend
  tails briefly; do not expect seamless time-switch crossfades.
- The reported tail is 12 seconds; it is an estimate, not a hard tail truncation.
- The preset menu applies starting points. Its displayed selection is not a
  reliable modified-state indicator, and the menu selection itself is not saved;
  the actual sound parameters are saved by the host.
- Prototype review found no deliberate audio-callback allocations; this is not a
  substitute for profiling, plug-in validation, or real-time stress testing.

## Validation before relying on it

1. Build both formats successfully and inspect all compiler warnings.
2. Run AU validation and a VST3 validator or a third-party plug-in validator.
3. Test your actual host on macOS 12; test native Intel/Apple Silicon as needed.
4. Exercise 44.1, 48, 88.2, and 96 kHz; varying block sizes; mono/stereo layouts;
   offline rendering; transport stops; automation; and state save/reload.
5. Compare full bypass, Mix = 0, and all effects at zero at matched loudness.
6. Check for clicks, denormals, CPU spikes, unexpectedly loud tails, and mono
   cancellation. Check delay sync with a known tempo and sparse test impulses.
7. Audition with real adlibs and review gain staging before using in paid work.

## Build troubleshooting

- `cmake: command not found`: install CMake and expose its bin directory in PATH.
- Compiler/SDK not found: install/select Apple Command Line Tools with xcode-select.
- FetchContent fails: check internet/Git access or supply a local JUCE folder.
- Changing compilers/generators: remove only this project's build folder and retry.
- Host does not see the plug-in: confirm the correct format, architecture, install
  location, successful validation, and the host's plug-in rescan procedure.
- A failed build is not a usable plug-in. Keep the full error log for debugging.

## Licensing

The original project source in Source/, build scripts and CMakeLists.txt is
provided under the MIT license in LICENSE.txt. JUCE is a separate dependency and
is NOT relicensed by this project. Comply with the license of the selected JUCE
version (GPL/commercial options as applicable), and review its current terms
before distributing binaries, especially closed-source commercial products.
No third-party audio samples, artist recordings, images, or impulse responses
are included.
