
const Tone = require('tone')

const note_names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

function handleMIDIPush (synth, data) {	
	let key = document.getElementById(data[1])
	if (key == null) return
	key.style.backgroundColor = 'red'
	let key_name = note_names[data[1] % 12] + String(Math.floor(data[1] / 12))
	synth.triggerAttack(key_name)
}

function handleMIDIRelease (synth, data) {	
	let key = document.getElementById(data[1])
	if (key == null) return
	let whities = [0, 2, 4, 5, 7, 9, 11]
	if (whities.includes(data[1]%12)) {
		key.style.backgroundColor = 'white'
	} else {
		key.style.backgroundColor = 'black'
	}
	let key_name = note_names[data[1] % 12] + String(Math.floor(data[1] / 12))
	synth.triggerRelease(key_name)
}

function gainMIDIAccess (navigator, synth) {
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
						handleMIDIPush(synth, midiMessage.data)
					} else if (midiMessage.data[0] == 128) {
						console.log(midiMessage.data)
						handleMIDIRelease(synth, midiMessage.data)
					}
				}
			}
		},
		() => {
			console.log('ERROR: MIDI access denied.')
		}
	)
}

const synth = new Tone.PolySynth().toDestination()
Tone.context.lookAhead = 0
synth.triggerAttackRelease(["C4", "E4", "A4"], 1)
console.log(synth.context.lookAhead)

gainMIDIAccess(window.navigator, synth)

