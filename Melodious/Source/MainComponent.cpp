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
					false), // hide advanced options
	loopProgressBar (progressInLoop)
{
  addAndMakeVisible (bgImage);

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

