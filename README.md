# Guitar Tuner

A real-time guitar tuner for Linux, written in C++17.

It listens to the microphone, detects the pitch with the YIN algorithm and shows the note and how many cents it is out of tune.

## Status

Work in progress.

- [x] Pitch detection with the YIN algorithm (difference function, cumulative mean normalized difference, absolute threshold, parabolic interpolation)
- [x] Frequency to note and cents conversion
- [ ] Live microphone input (RtAudio)
- [ ] Lock-free ring buffer between the audio thread and the analysis thread
- [ ] Qt/QML user interface
- [ ] Unit tests (GoogleTest) and CI

## Build

```bash
cmake -S . -B build
cmake --build build
./build/tuner
```

## References

- A. de Cheveigné, H. Kawahara, "YIN, a fundamental frequency estimator for speech and music", JASA, 2002.
