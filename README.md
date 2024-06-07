# PiMax
## Loudness Maximizer and Multiband Enhancer

A versatile tool for mixing and mastering.

### Features:

- 5 different saturation modes
- Symmetric & Asymmetric saturation
- Choice between full-band processing or up to 4 frequency bands
- Option for linear-phase multiband filters
- 4x oversampling with options for linear-phase filtering and deferring
  oversampling to offline rendering
- Stereo widening and mono-to-stereo tool

## Building

Requirements:

|Windows|MacOS|Linux|
|-------|-----|-----|
|Visual Studio|XCode Command Line Tools|See JUCE [Linux deps](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md)

To compile:

```sh
    cmake -Bbuild -DPRODUCTION_BUILD=1 -DINSTALL=1 -DNO_LICENSE_CHECK=1
    cmake --build build --config Release
```

## Credits

- [JUCE](https://github.com/juce-framework/juce)
- [PFFFT](https://github.com/marton78/pffft)
- [Gin](https://github.com/FigBug/gin)
- [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions)

See the manual under `manual/PiMax.pdf` for licensing info of these libraries

## License

PiMax is licensed under the GPLv3 license
