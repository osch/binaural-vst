.PHONY: build clean

build: source/hrir_kemar_bin.c JUCE/JuceLibraryCode
	cd JUCE/Builds/LinuxMakefile/; \
	make CONFIG=Release JUCE_VST3DESTDIR="" JUCE_LV2DESTDIR=""

JUCE/JuceLibraryCode: JUCE/binaural-vst.jucer
	Projucer --resave JUCE/binaural-vst.jucer

source/hrir_kemar_bin.c: build/hrir/kemar.bin source/kemarBinToC.lua
	cd source; lua kemarBinToC.lua
clean:
	rm -rf source/hrir_kemar_bin.c \
	       JUCE/JuceLibraryCode \
	       JUCE/Builds
