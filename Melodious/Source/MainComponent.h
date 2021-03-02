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
  SineWaveVoice (const juce::AudioSampleBuffer& wavetableToUse)
	: wavetable (wavetableToUse),
	  tableSize (wavetable.getNumSamples()) {}
  bool canPlaySound (juce::SynthesiserSound* sound) override
  {
	return dynamic_cast<SineWaveSound*> (sound) != nullptr;
  }

  void startNote (int midiNoteNumber, float velocity,
				  juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
  {
	currentIndex = 0.0;
	level = velocity * 0.15;
	tailOff = 0.0;

	auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
	tableDelta = (float) cyclesPerSecond / getSampleRate() * (float) tableSize;
  }

  void stopNote (float /*velocity*/, bool allowTailOff) override
  {
	if (allowTailOff)
	  {
		if (tailOff == 0.0)
		  tailOff = 1.0;
	  }
	else
	  {
		clearCurrentNote();
		tableDelta = 0.0;
	  }		  
  }

  void pitchWheelMoved (int) override {}
  void controllerMoved (int, int) override {}

  void renderNextBlock (juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
  {
	if (tableDelta != 0.0)
	  {
		while (--numSamples >= 0)
		  {
		    
			// auto currentSample = (float) (std::sin (currentAngle) * level * tailOff);
			// interpolating
			auto index0 = (unsigned int) currentIndex;
			auto index1 = index0 + 1;

			auto frac = currentIndex - (float) index0;

			auto table = wavetable.getReadPointer (0);
			auto value0 = table[index0];
			auto value1 = table[index1];

			auto currentSample = (float) (value0 + frac * (value1 - value0)) * level;
			
			// // using floor index
			// auto index0 = (unsigned int) currentIndex;
			// auto table = wavetable.getReadPointer (0);
			// auto currentSample = (float) table[index0] * level;
			
			if (tailOff > 0.0)
			  currentSample *= tailOff;

			for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
			  outputBuffer.addSample (i, startSample, currentSample);

			++startSample;

			if ((currentIndex += tableDelta) > (float) tableSize)
			  currentIndex -= (float) tableSize;

			if (tailOff > 0.0)
			  {
				tailOff *= 0.99; // [8]

				if (tailOff <= 0.005)
				  {
					clearCurrentNote(); // [9]

					tableDelta = 0.0;
					break;
				  }
			  }
		  }
	  }
  }
  
private:
  // double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
  double level = 0.0, tailOff = 0.0;
  const juce::AudioSampleBuffer& wavetable;
  const int tableSize;
  float currentIndex = 0.0f, tableDelta = 0.0f;
};

//==============================================================================
class SynthAudioSource   : public juce::AudioSource
{
public:
  SynthAudioSource (juce::MidiKeyboardState& keyState)
	: keyboardState (keyState)
  {
	createWavetable();
	
	for (auto i = 0; i < 4; ++i)                // [1]
	  synth.addVoice (new SineWaveVoice (sineTable));

	synth.addSound (new SineWaveSound());       // [2]
  }

  void setUsingSineWaveSound()
  {
	synth.clearSounds();
  }
  
  void createWavetable()
  {
	sineTable.setSize (1, (int) tableSize + 1);
	auto* samples = sineTable.getWritePointer (0);

	auto angleDelta = juce::MathConstants<double>::twoPi / (double) (tableSize -1);
	auto currentAngle = 0.0;

	for (unsigned int i = 0; i < tableSize; ++i)
	  {
		auto sample = std::sin (currentAngle);
		samples[i] = (float) sample;
		currentAngle += angleDelta;
	  }
	samples[tableSize] = samples[0];
  }

  void setupLoop(double spl) {
	samplePerLoop = spl;
	for (int i = 0; i < 2; i++) {
	  // scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 53, 1.0f), i*samplePerLoop);
	  // scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 53), std::floor(samplePerLoop/4) + i*samplePerLoop);
	  // scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 53, 1.0f), std::floor(samplePerLoop*3/8) + i*samplePerLoop);
	  // scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 53), std::floor(samplePerLoop/2) + i*samplePerLoop - 512);
	  // scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 60, 1.0f), std::floor(samplePerLoop/2) + i*samplePerLoop);
	  // scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 60), std::floor(samplePerLoop*3/4) + i*samplePerLoop);
	  // scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 60, 1.0f), std::floor(samplePerLoop*7/8) + i*samplePerLoop);
	  // scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 60), std::floor(samplePerLoop) + i*samplePerLoop - 512);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 59, 1.0f), i * samplePerLoop);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 53, 1.0f), i * samplePerLoop + 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 43, 1.0f), i * samplePerLoop + 2);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 59), std::floor(samplePerLoop*5/24) + i * samplePerLoop - 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 53), std::floor(samplePerLoop*5/24) + i * samplePerLoop - 2);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 43), std::floor(samplePerLoop*5/24) + i * samplePerLoop - 3);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 31, 1.0f), std::floor(samplePerLoop*5/24) + i * samplePerLoop);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 31), std::floor(samplePerLoop*1/4) + i * samplePerLoop - 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 43, 1.0f), std::floor(samplePerLoop*1/2) + i * samplePerLoop);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 43), std::floor(samplePerLoop*17/24) + i * samplePerLoop - 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 31, 1.0f), std::floor(samplePerLoop*17/24) + i * samplePerLoop);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 31), std::floor(samplePerLoop*3/4) + i * samplePerLoop - 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 36, 1.0f), std::floor(samplePerLoop*20/24) + i * samplePerLoop);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 36), std::floor(samplePerLoop*7/8) + i * samplePerLoop - 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 37, 1.0f), std::floor(samplePerLoop*7/8) + i * samplePerLoop);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 37), std::floor(samplePerLoop*23/24) + i * samplePerLoop - 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 38, 1.0f), std::floor(samplePerLoop*23/24) + i * samplePerLoop);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 38), (i+1) * samplePerLoop - 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 54, 1.0f), std::floor(samplePerLoop*23/24) + i * samplePerLoop + 1);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 54), (i+1) * samplePerLoop - 2);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOn(0, 58, 1.0f), std::floor(samplePerLoop*23/24) + i * samplePerLoop + 2);
	  scriptedMidiEvents.addEvent(juce::MidiMessage::noteOff(0, 58), (i+1) * samplePerLoop - 3);
	}
  }

  void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
  {
	synth.setCurrentPlaybackSampleRate (sampleRate); // [3]
	midiCollector.reset (sampleRate);
	setupLoop (sampleRate*5);	   
  }

  void releaseResources() override {}

  void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
  {
	bufferToFill.clearActiveBufferRegion();

	juce::MidiBuffer incomingMidi;
	midiCollector.removeNextBlockOfMessages (incomingMidi, bufferToFill.numSamples);
	keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,
										 bufferToFill.numSamples, true);       // [4]
	// Adding scripted midi events
	incomingMidi.addEvents(scriptedMidiEvents, bufferToFill.numSamples*currentCycle % samplePerLoop, bufferToFill.numSamples, 0);
	synth.renderNextBlock (*bufferToFill.buffer, incomingMidi,
						   bufferToFill.startSample, bufferToFill.numSamples); // [5]
	currentCycle++;
	if (currentCycle>=samplePerLoop)
	  currentCycle -= samplePerLoop;
  }
  juce::MidiMessageCollector* getMidiCollector()
  {
	return &midiCollector;
  }

private:
  
  juce::AudioSampleBuffer sineTable;
  const unsigned int tableSize = 1 << 7;
  
  juce::MidiKeyboardState& keyboardState;
  juce::Synthesiser synth;
  juce::MidiMessageCollector midiCollector;
  int currentCycle = 0;
  juce::MidiBuffer scriptedMidiEvents;
  int samplePerLoop;
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

  //==============================================================================
  void paint (juce::Graphics& g) override;
  void timerCallback() override;
  void resized() override;

private:
  void changeListenerCallback (juce::ChangeBroadcaster*) override {}
  //==============================================================================
  // Your private member variables go here...
  juce::MidiKeyboardState keyboardState;
  SynthAudioSource synthAudioSource;
  juce::MidiKeyboardComponent keyboardComponent;

  juce::ComboBox midiInputList;
  juce::Label midiInputListLabel;
  int lastInputIndex = 0;
  juce::AudioDeviceSelectorComponent audioSetupComp;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
