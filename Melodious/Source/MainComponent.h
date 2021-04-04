#pragma once

#include <JuceHeader.h>

struct SineWaveSound : public juce::SynthesiserSound
{
  SineWaveSound() {}

  bool appliesToNote (int) override { return true; }
  bool appliesToChannel (int) override { return true; }
};

struct SineWaveVoice : public juce::SynthesiserVoice
{
  SineWaveVoice (const juce::AudioSampleBuffer&);
  bool canPlaySound (juce::SynthesiserSound* sound) override;
  void startNote (int, float, juce::SynthesiserSound*, int) override;
  void stopNote (float, bool) override;
  void pitchWheelMoved (int) override {}
  void controllerMoved (int, int) override {}
  void renderNextBlock (juce::AudioSampleBuffer&, int, int) override;
  
private:
  double level = 0.0, tailOff = 0.0;
  const juce::AudioSampleBuffer& wavetable;
  const int tableSize;
  float currentIndex = 0.0f, tableDelta = 0.0f;
};

class CircularProgressBarLaF : public juce::LookAndFeel_V4
{
public:
  CircularProgressBarLaF (float);

  void drawProgressBar (juce::Graphics&, juce::ProgressBar&,
						int, int, double, const juce::String&) override;
  private:
	  float elevation;
};

//==============================================================================

class TitleBeltComponent : public juce::Component
{
public:
  TitleBeltComponent (const juce::String& titleStr, bool align = true)
	: titleString (titleStr),
	  alignLeft (align) {}
  void paint (juce::Graphics&) override;
  
private:
  juce::String titleString;
  bool alignLeft;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TitleBeltComponent);
};

//==============================================================================
class LooperAudioSource   : public juce::AudioSource
{
public:
  LooperAudioSource (juce::MidiKeyboardState&);
  void setUsingSineWaveSound();
  void setupSamplesPerLoop (int);
  void loadPhrases();
  void savePhrases();
  void createWavetable();
  void setupPhrase();  
  void setupRythmSection();
  void evaluateGuess();
  void prepareToPlay (int, double) override;  
  void releaseResources() override {}
  void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;    
  juce::MidiMessageCollector* getMidiCollector();

private:
  
  juce::AudioSampleBuffer sineTable;
  const unsigned int tableSize = 1 << 7;
  
  juce::MidiKeyboardState& keyboardState;
  juce::Synthesiser synth;
  juce::MidiMessageCollector midiCollector;
  int currentCyclePos = 0, currentPhase = 1; // currentPhase = 0 for none, 1 for computer playing phrase, 2 for listening to user input
  // TODO: const static members for these values
  juce::MidiBuffer rythmSectionBuffer, phraseBuffer, guessBuffer;
  juce::MidiBuffer phrases[10];
  int samplesPerLoop;
};


//==============================================================================
/*
  This component lives inside our window, and this is where you should put all
  your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
					   public juce::ChangeListener,
					   public juce::Timer
{
public:
  //==============================================================================
  MainComponent();
  ~MainComponent() override;

  //==============================================================================
  void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
  void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
  void releaseResources() override;
  void setMidiInput (int);
  void loadBgImage();

  //==============================================================================
  void paint (juce::Graphics& g) override;
  void timerCallback() override;
  void resized() override;

private:
  void changeListenerCallback (juce::ChangeBroadcaster*) override {}
  //==============================================================================
  juce::MidiKeyboardState keyboardState;
  LooperAudioSource synthAudioSource;
  juce::MidiKeyboardComponent keyboardComponent;

  juce::ComboBox midiInputList;
  juce::Label midiInputListLabel;
  juce::ImageComponent bgImage;
  int lastInputIndex = 0;
  juce::AudioDeviceSelectorComponent audioSetupComp;
  int timerCounter = 0;
  int timerHz = 60;
  double progressInLoop = 0.0;
  int secondsPerLoop;
  juce::ProgressBar loopProgressBar;
  CircularProgressBarLaF progressBarLaF;
  TitleBeltComponent chordName;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
