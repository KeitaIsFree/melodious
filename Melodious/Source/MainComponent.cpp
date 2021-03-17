#include "MainComponent.h"

//----------------------------------------------------------------------------------------------------

SineWaveVoice::SineWaveVoice (const juce::AudioSampleBuffer& wavetableToUse)
  : wavetable (wavetableToUse),
	tableSize (wavetable.getNumSamples()) {}

bool SineWaveVoice::canPlaySound (juce::SynthesiserSound* sound)
{
  return dynamic_cast<SineWaveSound*> (sound) != nullptr;
}

void SineWaveVoice::startNote (int midiNoteNumber, float velocity,
				juce::SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
  currentIndex = 0.0;
  level = velocity * 0.15;
  tailOff = 0.0;

  auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
  tableDelta = (float) cyclesPerSecond / getSampleRate() * (float) tableSize;
}

void SineWaveVoice::stopNote (float /*velocity*/, bool allowTailOff)
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

void SineWaveVoice::renderNextBlock (juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
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

//----------------------------------------------------------------------------------------------------

CircularProgressBarLaF::CircularProgressBarLaF (float elv = 0.6f)
  : elevation (elv) {}

void CircularProgressBarLaF::drawProgressBar (juce::Graphics& g, juce::ProgressBar& progressBar,
											  int width, int height, double progress,
											  const juce::String&)
{
  const juce::Colour background (progressBar.findColour (juce::ProgressBar::backgroundColourId));
  const juce::Colour foreground (progressBar.findColour (juce::ProgressBar::foregroundColourId));
	
  auto barBounds = progressBar.getLocalBounds();
  juce::Path inner;
  inner.addCentredArc (barBounds.getCentreX(),
					   barBounds.getCentreY(),
					   barBounds.getWidth() * 0.5f * elevation,
					   barBounds.getHeight() * 0.5f * elevation,
					   0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
  g.setColour (background);
  g.strokePath (inner, juce::PathStrokeType (barBounds.getWidth() / 5));
  juce::Path outer;
  outer.addCentredArc (barBounds.getCentreX(),
					   barBounds.getCentreY(),
					   barBounds.getWidth() * 2.0f / 5.0f,
					   barBounds.getHeight() * 2.0f / 5.0f,
					   0.0f,
					   0.0f,
					   juce::MathConstants<float>::twoPi * progress,
					   true);
  g.setColour (foreground);
  g.strokePath (outer, juce::PathStrokeType (barBounds.getWidth() / 5));
}

//============================================================================

void TitleBeltComponent::paint (juce::Graphics& g)
{
  g.fillAll (juce::Colour (0, 71, 87));
  g.setColour (juce::Colours::white);
  g.drawLine (8, 12, getWidth() - 8, 12, 8);
  g.drawLine (8, getHeight() - 12, getWidth() - 8,  getHeight() - 12, 8);
  g.setFont (96);
  g.drawText (titleString, 8, 12, getWidth() - 8, getHeight() - 12, juce::Justification::Flags::left);
}

//----------------------------------------------------------------------------
LooperAudioSource::LooperAudioSource (juce::MidiKeyboardState& keyState)
  : keyboardState (keyState)
{
  createWavetable();
	
  for (auto i = 0; i < 4; ++i)                // [1]
	synth.addVoice (new SineWaveVoice (sineTable));

  synth.addSound (new SineWaveSound());       // [2]
}

void LooperAudioSource::setUsingSineWaveSound()
{
  synth.clearSounds();
}

void LooperAudioSource::setupSamplesPerLoop (int spl) {
  std::cout << "Setting up samples per loop: " << spl << "\n";
  samplesPerLoop = spl;
}
  
void LooperAudioSource::createWavetable()
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

void LooperAudioSource::setupPhrase () {
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
  
void LooperAudioSource::setupRythmSection () {
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

void LooperAudioSource::evaluateGuess()
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

void LooperAudioSource::prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate)
{
  synth.setCurrentPlaybackSampleRate (sampleRate); // [3]
  midiCollector.reset (sampleRate);
  setupRythmSection ();
  setupPhrase();
}

void LooperAudioSource::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
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
    
juce::MidiMessageCollector* LooperAudioSource::getMidiCollector()
{
  return &midiCollector;
}


//==============================================================================
MainComponent::MainComponent()
  : synthAudioSource (keyboardState),
	keyboardComponent (keyboardState, juce::MidiKeyboardComponent::verticalKeyboardFacingLeft),
	audioSetupComp (deviceManager,
					0,     // minimum input channels
					256,   // maximum input channels
					0,     // minimum output channels
					256,   // maximum output channels
					false, // ability to select midi inputs
					false, // ability to select midi output device
					false, // treat channels as stereo pairs
					false), // hide advanced options
	loopProgressBar (progressInLoop),
	chordName ("Cm7b5")
{
  addAndMakeVisible (bgImage);

  addAndMakeVisible (chordName);

  addAndMakeVisible (keyboardComponent);

  loopProgressBar.setLookAndFeel (&progressBarLaF);

  loopProgressBar.setColour (juce::ProgressBar::ColourIds::foregroundColourId,
							 juce::Colour (255, 118, 118));
  loopProgressBar.setColour (juce::ProgressBar::ColourIds::backgroundColourId,
							 juce::Colour (84, 84, 84));
  loopProgressBar.setPercentageDisplay (false);

  addAndMakeVisible (loopProgressBar);

  // addAndMakeVisible (audioSetupComp);

  // Make sure you set the size of the component after
  // you add any child components.
  setSize (1920, 1080);

  loadBgImage();
  
  setAudioChannels (0, 2);

  secondsPerLoop = 5;

  startTimerHz (timerHz);
  // addAndMakeVisible (midiInputListLabel);
  midiInputListLabel.setText ("MIDI Input:", juce::dontSendNotification);
  midiInputListLabel.attachToComponent (&midiInputList, true);
 
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  // addAndMakeVisible (midiInputList);
  midiInputList.setTextWhenNoChoicesAvailable ("No MIDI Inputs Enabled");
 
  juce::StringArray midiInputNames;
  for (auto input : midiInputs)
	midiInputNames.add (input.name);
 
  midiInputList.addItemList (midiInputNames, 1);
  midiInputList.onChange = [this] { setMidiInput (midiInputList.getSelectedItemIndex()); };
 
  for (auto input : midiInputs)
	{
	  if (deviceManager.isMidiInputDeviceEnabled (input.identifier))
		{
		  setMidiInput (midiInputs.indexOf (input));
		  break;
		}
	}
 
  if (midiInputList.getSelectedId() == 0)
	setMidiInput (0);
  
}

MainComponent::~MainComponent()
{
  // This shuts down the audio device and clears the audio source.
  shutdownAudio();
}

void MainComponent::setMidiInput (int index)
{
  auto list = juce::MidiInput::getAvailableDevices();
 
  deviceManager.removeMidiInputDeviceCallback (list[lastInputIndex].identifier, synthAudioSource.getMidiCollector());
 
  auto newInput = list[index];
 
  if (! deviceManager.isMidiInputDeviceEnabled (newInput.identifier))
	deviceManager.setMidiInputDeviceEnabled (newInput.identifier, true);
 
  deviceManager.addMidiInputDeviceCallback (newInput.identifier, synthAudioSource.getMidiCollector());
  midiInputList.setSelectedId (index + 1, juce::dontSendNotification);
 
  lastInputIndex = index;
}

void MainComponent::loadBgImage()
{
  const auto bgImageFile = juce::File ("/home/roy/Code/melodious/Melodious/Source/res/houses.png");
  if (bgImageFile.exists())
	{
	  juce::FileInputStream inputStreamRef (bgImageFile);
	  if (inputStreamRef.openedOk())
		bgImage.setImage (juce::PNGImageFormat::loadFrom (inputStreamRef), juce::RectanglePlacement (128));
	  else
		std::cout << "ERROR: Problem opening input stream for background image";
	}
  else
	std::cout << "ERROR: Background image file does not exist\n";
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
  // This function will be called when the audio device is started, or when
  // its settings (i.e. sample rate, block size, etc) are changed.

  // You can use this function to initialise any resources you might need,
  // but be careful - it will be called on the audio thread, not the GUI thread.

  // For more details, see the help for AudioProcessor::prepareToPlay()
  std::cout << "sampleRate: " << sampleRate << ", secondsPerLoop: " << secondsPerLoop << "\n";
  // for some reason, secondsPerLoop is 0 when accessed from here
  secondsPerLoop = 5;
  synthAudioSource.setupSamplesPerLoop (sampleRate * secondsPerLoop);
  synthAudioSource.prepareToPlay (samplesPerBlockExpected, sampleRate);

  
  Timer::callAfterDelay (400,
						[&] () { keyboardComponent.grabKeyboardFocus(); });
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
  // Your audio-processing code goes here!

  // For more details, see the help for AudioProcessor::getNextAudioBlock()

  // Right now we are not producing any data, in which case we need to clear the buffer
  // (to prevent the output of random noise)
  synthAudioSource.getNextAudioBlock (bufferToFill);
}

void MainComponent::releaseResources()
{
  // This will be called when the audio device stops, or when it is being
  // restarted due to a setting change.

  // For more details, see the help for AudioProcessor::releaseResources()
  synthAudioSource.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

}

void MainComponent::timerCallback()
{
  // Animation update here
  timerCounter++;
  progressInLoop = (float) (timerCounter % (timerHz * secondsPerLoop)) / (float) (timerHz * secondsPerLoop);
  if (timerCounter % (timerHz * secondsPerLoop) == 0)
	{
	  if ((timerCounter % (timerHz * secondsPerLoop * 2)) == timerHz * secondsPerLoop)
		loopProgressBar.setColour (juce::ProgressBar::ColourIds::foregroundColourId,
								   juce::Colour (20, 255, 0));
	  else
		loopProgressBar.setColour (juce::ProgressBar::ColourIds::foregroundColourId,
								   juce::Colour (255, 118, 118));
	}
  
}

void MainComponent::resized()
{
  // This is called when the MainContentComponent is resized.
  // If you add any child components, this is where you should
  // update their positions.auto rect = getLocalBounds();

  chordName.setBounds (getWidth() - 400, 100, 400, 128);
  bgImage.setBounds (0, 0, getWidth(), getHeight());
  auto rect = getLocalBounds();
  audioSetupComp.setBounds (rect.removeFromLeft (proportionOfWidth (0.6f)));
  midiInputList    .setBounds (200, 10, getWidth() - 210, 20);
  keyboardComponent.setKeyWidth ((float) getHeight() / (float) 52);
  keyboardComponent.setLowestVisibleKey (21);
  keyboardComponent.setAvailableRange (21, 108);
  keyboardComponent.setBounds (0, 0, 100, getHeight());
  loopProgressBar.setBounds(getWidth() - 100, getHeight() - 100, 80, 80);
}

