

let audioContext = new (window.AudioContext || window.webkitAudioContext)();
let oscList = [];
let masterGainNode = null;
let customWaveform = null;
let sineTerms = null;
let cosineTerms = null;

let cOsc = audioContext.createOscillator();

masterGainNode = audioContext.createGain();
masterGainNode.connect(audioContext.destination);
masterGainNode.gain.value = 1.0

const note_names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

let noteFreq = [];
for (let i = 0; i < 9; i++) {
	noteFreq[i] = []
	console.log(noteFreq)
}
noteFreq[0]["A"] = 27.500000000000000;
noteFreq[0]["A#"] = 29.135235094880619;
noteFreq[0]["B"] = 30.867706328507756;

noteFreq[1]["C"] = 32.703195662574829;
noteFreq[1]["C#"] = 34.647828872109012;
noteFreq[1]["D"] = 36.708095989675945;
noteFreq[1]["D#"] = 38.890872965260113;
noteFreq[1]["E"] = 41.203444614108741;
noteFreq[1]["F"] = 43.653528929125485;
noteFreq[1]["F#"] = 46.249302838954299;
noteFreq[1]["G"] = 48.999429497718661;
noteFreq[1]["G#"] = 51.913087197493142;
noteFreq[1]["A"] = 55.000000000000000;
noteFreq[1]["A#"] = 58.270470189761239;
noteFreq[1]["B"] = 61.735412657015513;

for (let i = 2; i < 9; i++) {
	for (let j = 0; j < 12; j++) {
		noteFreq[i][note_names[j]] = noteFreq[i-1][note_names[j]] * 2;
	}
}


function handleMIDIPush (data) {
	let startTime = audioContext.currentTime
	let key = document.getElementById(data[1])
	if (key == null) return
	key.style.backgroundColor = 'red'
	let octave = Math.floor(data[1] / 12)
	let note = note_names[data[1] % 12]
	let key_name = note + String(octave)
	oscList[octave][note] = playTone(noteFreq[octave][note])
	console.log( audioContext.currentTime - startTime)
}

function handleMIDIRelease (data) {	
	let key = document.getElementById(data[1])
	if (key == null) return
	let whities = [0, 2, 4, 5, 7, 9, 11]
	if (whities.includes(data[1]%12)) {
		key.style.backgroundColor = 'white'
	} else {
		key.style.backgroundColor = 'black'
	}
	let octave = Math.floor(data[1] / 12)
	let note = note_names[data[1] % 12]
	let key_name = note + String(octave)
	oscList[octave][note].stop();
	delete oscList[octave][note];
}

function gainMIDIAccess (navigator) {
	navigator.requestMIDIAccess().then(
		(midiAccess) => {
			var midi_inputs = midiAccess.inputs;
			var midi_outputs = midiAccess.outputs;
			console.log('MIDI access granted.: ')
			for (let input of midi_inputs.values()) {
				input.onmidimessage = (midiMessage) => {
						console.log(midiMessage.data)
					if (midiMessage.data[0] == 144) {
						console.log(midiMessage.data)
						handleMIDIPush(midiMessage.data)
					} else if (midiMessage.data[0] == 128) {
						console.log(midiMessage.data)
						handleMIDIRelease(midiMessage.data)
					}
				}
			}
		},
		() => {
			console.log('ERROR: MIDI access denied.')
		}
	)
}

function setup() {
	
	sineTerms = new Float32Array([0, 0, 1, 0, 1]);
	cosineTerms = new Float32Array(sineTerms.length);
	customWaveform = audioContext.createPeriodicWave(cosineTerms, sineTerms);
	
	for (let i=0; i<9; i++) {
		oscList[i] = {};
	}
}

function playTone(freq) {
	let osc = audioContext.createOscillator();
	osc.connect(masterGainNode);

	osc.type = "sine";

	osc.frequency.value = freq;
	osc.start();

	return osc;
}

setup()

gainMIDIAccess(window.navigator)


