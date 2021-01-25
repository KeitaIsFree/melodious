
function handleMIDIPush (data) {	
	let key = document.getElementById(data[1])
	if (key == null) return
	key.style.backgroundColor = 'red'
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
}

function gainMIDIAccess (navigator) {
	navigator.requestMIDIAccess().then(
		(midiAccess) => {
			var midi_inputs = midiAccess.inputs;
			var midi_outputs = midiAccess.outputs;
			console.log('MIDI access granted.: ')
			for (let input of midi_inputs.values()) {
				input.onmidimessage = (midiMessage) => {
					if (midiMessage.data[0] == 144) {
						console.log(midiMessage.data)
						handleMIDIPush(midiMessage.data)
					} else if (midiMessage.data[0] == 128) {
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


gainMIDIAccess(window.navigator)
