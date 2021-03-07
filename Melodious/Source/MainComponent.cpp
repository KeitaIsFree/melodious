#include "MainComponent.h"


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
					false) // hide advanced options
{

  addAndMakeVisible (keyboardComponent);

  addAndMakeVisible (audioSetupComp);
  // Make sure you set the size of the component after
  // you add any child components.
  setSize (1920, 1080);

  // Some platforms require permissions to open input channels so request that here
  // if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
  // 	  && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
  //   {
  // 	  juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
  // 										 [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
  //   }
  // else
  //   {
  // 	  // Specify the number of input and output channels that we want to open
  // 	  setAudioChannels (2, 2);
  //   }
  
  setAudioChannels (0, 2);

  secondsPerLoop = 5;

  startTimerHz (10);
  addAndMakeVisible (midiInputListLabel);
  midiInputListLabel.setText ("MIDI Input:", juce::dontSendNotification);
  midiInputListLabel.attachToComponent (&midiInputList, true);
 
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  addAndMakeVisible (midiInputList);
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

  
  keyboardComponent.grabKeyboardFocus();
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

  // You can add your drawing code here!
  juce::Path spinePath;
  int numberOfDots = 60;
  for (auto i = 0; i < numberOfDots; ++i) // [3]
	{
	  int radius = 150;
 
	  juce::Point<float> p ((float) getWidth()  / 2.0f + 1.0f * (float) radius * std::sin ((float) (timerCounter % (10*secondsPerLoop)) / 10.0f / (float) secondsPerLoop * juce::MathConstants<float>::twoPi * i / numberOfDots),
							(float) getHeight() / 2.0f + 1.0f * (float) radius * std::cos ((float) (timerCounter % (10*secondsPerLoop)) / 10.0f / (float) secondsPerLoop * juce::MathConstants<float>::twoPi * i / numberOfDots));
 
	  if (i == 0)
		spinePath.startNewSubPath (p);  // if this is the first point, start a new path..
	  else
		spinePath.lineTo (p);           // ...otherwise add the next point
	}
  g.strokePath (spinePath, juce::PathStrokeType (4.0f)); // [4]
  
}

void MainComponent::timerCallback()
{
  // Animation update here
  timerCounter++;
  repaint();
  std::cout << "timerCounter: " << timerCounter << "\n";
  std::cout << "angleInLoop: " << (float) timerCounter / 60.0f / (float) secondsPerLoop * juce::MathConstants<float>::twoPi << "\n";
}

void MainComponent::resized()
{
  // This is called when the MainContentComponent is resized.
  // If you add any child components, this is where you should
  // update their positions.auto rect = getLocalBounds();

  auto rect = getLocalBounds();
  audioSetupComp.setBounds (rect.removeFromLeft (proportionOfWidth (0.6f)));
  midiInputList    .setBounds (200, 10, getWidth() - 210, 20);
  keyboardComponent.setKeyWidth ((float) getHeight() / (float) 52);
  keyboardComponent.setLowestVisibleKey (21);
  keyboardComponent.setAvailableRange (21, 108);
  keyboardComponent.setBounds (0, 0, 100, getHeight());
}
