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
class LooperAudioSource   : public juce::AudioSource
{
public:
  LooperAudioSource (juce::MidiKeyboardState& keyState)
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

  void setupSamplesPerLoop (int spl) {
	std::cout << "Setting up samples per loop: " << spl << "\n";
	samplesPerLoop = spl;
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

  void setupPhrase () {
	phraseBuffer.clear();
	auto one12thNote = std::floor(samplesPerLoop / 12 / 2);
	phraseBuffer.addEvent (juce::MidiMessage::noteOn (1, 67, 1.0f), 0);
	phraseBuffer.addEvent (juce::MidiMessage::noteOff (1, 67), one12thNote * 2 - 1);
	phraseBuffer.addEvent (juce::MidiMessage::noteOn (1, 69, 1.0f), one12thNote * 2);
	phraseBuffer.addEvent (juce::MidiMessage::noteOff (1, 69), one12thNote * 3 - 1);
	phraseBuffer.addEvent (juce::MidiMessage::noteOn (1, 70, 1.0f), one12thNote * 3);
	phraseBuffer.addEvent (juce::MidiMessage::noteOff (1, 70), one12thNote * 5 - 1);
	phraseBuffer.addEvent (juce::MidiMessage::noteOn (1, 72, 1.0f), one12thNote * 5);
	phraseBuffer.addEvent (juce::MidiMessage::noteOff (1, 72), one12thNote * 6 - 1);
	phraseBuffer.addEvent (juce::MidiMessage::noteOn (1, 69, 1.0f), one12thNote * 6);
	phraseBuffer.addEvent (juce::MidiMessage::noteOff (1, 69), one12thNote * 9 - 1);
	phraseBuffer.addEvent (juce::MidiMessage::noteOn (1, 65, 1.0f), one12thNote * 9);
	phraseBuffer.addEvent (juce::MidiMessage::noteOff (1, 65), one12thNote * 11 - 1);
	phraseBuffer.addEvent (juce::MidiMessage::noteOn (1, 67, 1.0f), one12thNote * 11);
	phraseBuffer.addEvent (juce::MidiMessage::noteOff (1, 67), one12thNote *12 - 1);
  }
  
  void setupRythmSection () {
    rythmSectionBuffer.clear();
	for (int i = 0; i < 2; i++) {
	  // rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 53, 1.0f), i*samplesPerLoop);
	  // rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 53), std::floor(samplesPerLoop/4) + i*samplesPerLoop);
	  // rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 53, 1.0f), std::floor(samplesPerLoop*3/8) + i*samplesPerLoop);
	  // rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 53), std::floor(samplesPerLoop/2) + i*samplesPerLoop - 512);
	  // rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 60, 1.0f), std::floor(samplesPerLoop/2) + i*samplesPerLoop);
	  // rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 60), std::floor(samplesPerLoop*3/4) + i*samplesPerLoop);
	  // rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 60, 1.0f), std::floor(samplesPerLoop*7/8) + i*samplesPerLoop);
	  // rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 60), std::floor(samplesPerLoop) + i*samplesPerLoop - 512);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 59, 1.0f), i * samplesPerLoop);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 53, 1.0f), i * samplesPerLoop + 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 43, 1.0f), i * samplesPerLoop + 2);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 59), std::floor(samplesPerLoop*5/24) + i * samplesPerLoop - 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 53), std::floor(samplesPerLoop*5/24) + i * samplesPerLoop - 2);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 43), std::floor(samplesPerLoop*5/24) + i * samplesPerLoop - 3);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 31, 1.0f), std::floor(samplesPerLoop*5/24) + i * samplesPerLoop);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 31), std::floor(samplesPerLoop*1/4) + i * samplesPerLoop - 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 43, 1.0f), std::floor(samplesPerLoop*1/2) + i * samplesPerLoop);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 43), std::floor(samplesPerLoop*17/24) + i * samplesPerLoop - 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 31, 1.0f), std::floor(samplesPerLoop*17/24) + i * samplesPerLoop);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 31), std::floor(samplesPerLoop*3/4) + i * samplesPerLoop - 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 36, 1.0f), std::floor(samplesPerLoop*20/24) + i * samplesPerLoop);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 36), std::floor(samplesPerLoop*7/8) + i * samplesPerLoop - 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 37, 1.0f), std::floor(samplesPerLoop*7/8) + i * samplesPerLoop);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 37), std::floor(samplesPerLoop*23/24) + i * samplesPerLoop - 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 38, 1.0f), std::floor(samplesPerLoop*23/24) + i * samplesPerLoop);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 38), (i+1) * samplesPerLoop - 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 54, 1.0f), std::floor(samplesPerLoop*23/24) + i * samplesPerLoop + 1);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 54), (i+1) * samplesPerLoop - 2);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOn(0, 58, 1.0f), std::floor(samplesPerLoop*23/24) + i * samplesPerLoop + 2);
	  rythmSectionBuffer.addEvent(juce::MidiMessage::noteOff(0, 58), (i+1) * samplesPerLoop - 3);
	}
  }

  void evaluateGuess()
  {
	// int samplesGotRight = 0;
	// int samplesInPhrase = 0;
	int notesGotRight = 0, notesInTotal = 0;
	juce::MidiBufferIterator phraseIterator = phraseBuffer.begin();
	for (auto currentMidiMessageMetadata : phraseBuffer) {
	  auto currentMidiEvent = (*phraseIterator).getMessage();
	  // std::cout << "-------------------------------------\n";
	  // std::cout << "currentMidiEvent: " << currentMidiEvent.getDescription() <<"\n";
	  auto correctNote = (currentMidiEvent.isNoteOn())
		? currentMidiEvent.getNoteNumber()
		: 0;
	  auto noteFrom = currentMidiEvent.getTimeStamp();
	  auto noteTo = (*(++phraseIterator)).getMessage().getTimeStamp();
	  // std::cout << "Note duration is from " << noteFrom << " to " << noteTo << "\n";
	  if (correctNote == 0)
		continue;
	  // samplesInPhrase += noteTo - noteFrom;
	  int samplesGotRight = 0;
	  auto guessIterator = guessBuffer.findNextSamplePosition (noteFrom);
	  for (auto currentGuessMidiMessageMetadata : guessBuffer) {
		auto currentGuessMidiEvent = (*guessIterator).getMessage();
		// std::cout << "currentGuessMidiEvent: " << currentGuessMidiEvent.getDescription() <<"\n";
		if (noteTo < currentGuessMidiEvent.getTimeStamp()) {
		  break;
		}
		if (!currentGuessMidiEvent.isNoteOn()
			|| currentGuessMidiEvent.getNoteNumber() != correctNote) {
		  ++guessIterator;
		  continue;
		}
		// TODO: handle events other than noteOn/noteOff
		auto nextGuessMidiEvent = (*(++guessIterator)).getMessage();
		while (!nextGuessMidiEvent.isNoteOn()
			   && nextGuessMidiEvent.getNoteNumber() != correctNote)
		  nextGuessMidiEvent = (*(++guessIterator)).getMessage();		  
		// std::cout << "Note duration is from " << currentGuessMidiEvent.getTimeStamp() << " to " << nextGuessMidiEvent.getTimeStamp() << "\n";
		if (nextGuessMidiEvent.isNoteOff()) {
		  if (noteTo > nextGuessMidiEvent.getTimeStamp()) {
			samplesGotRight += nextGuessMidiEvent.getTimeStamp() - currentGuessMidiEvent.getTimeStamp();
			++guessIterator;
			continue;
		  } else {
			samplesGotRight += noteTo - currentGuessMidiEvent.getTimeStamp();
			break;
		  }
		} else {
		  if (noteTo > nextGuessMidiEvent.getTimeStamp()) {
			samplesGotRight += nextGuessMidiEvent.getTimeStamp() - currentGuessMidiEvent.getTimeStamp();
			continue;
		  } else {
			samplesGotRight += noteTo - currentGuessMidiEvent.getTimeStamp();
			break;
		  }
		}
			
	  }
	  // std::cout << "Note: " << currentMidiEvent.getDescription() << "\n";
	  // std::cout << (float) samplesGotRight / (float) (noteTo - noteFrom) << "\n";
	  if ((float) samplesGotRight / (float) (noteTo - noteFrom) > 0.4)
		notesGotRight++;
	  notesInTotal++;
	}
	// std::cout << "You got " << notesGotRight << " out of "<< notesInTotal << " notes right this loop.\n";
  }

  void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
  {
	synth.setCurrentPlaybackSampleRate (sampleRate); // [3]
	midiCollector.reset (sampleRate);
  	setupRythmSection ();
  	setupPhrase();
  }

  void releaseResources() override {}

  void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
  {
	// std::cout << currentPhase << "\n";
	bufferToFill.clearActiveBufferRegion();

	juce::MidiBuffer incomingMidi;
	midiCollector.removeNextBlockOfMessages (incomingMidi, bufferToFill.numSamples);
	keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,
										 bufferToFill.numSamples, true);       // [4]
	
	// Adding scripted midi events
	incomingMidi.addEvents(rythmSectionBuffer, currentCyclePos, bufferToFill.numSamples, 0);
	switch (currentPhase) {
	case 1: 
	  incomingMidi.addEvents (phraseBuffer, currentCyclePos, bufferToFill.numSamples, 0);
	  break;
	case 2:
	  // std::cout << "Listening... (currentPhase: 1)\n";
	  guessBuffer.addEvents (incomingMidi, 0, bufferToFill.numSamples, currentCyclePos);
	  break;
	default:
	  // std::cout << "Waiting... (currentPhase: 0)\n";
	  break;
	}
	synth.renderNextBlock (*bufferToFill.buffer, incomingMidi,
						   bufferToFill.startSample, bufferToFill.numSamples); // [5]
	currentCyclePos += bufferToFill.numSamples;
	if (currentCyclePos >= samplesPerLoop)
	  {
		currentCyclePos -= samplesPerLoop;
		if (currentPhase==1)
		  currentPhase = 2;
		else
		  {
			currentPhase = 1;
			evaluateGuess();
			guessBuffer.clear();
		  }
	  }
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
  int currentCyclePos = 0, currentPhase = 1; // currentPhase = 0 for none, 1 for computer playing phrase, 2 for listening to user input
  // TODO: constants or macro for these values
  juce::MidiBuffer rythmSectionBuffer, phraseBuffer, guessBuffer;
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
  // Your private member variables go here...
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

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
