const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline')

class PhysicalControls {
    // one PhysicalControls object per each arduino connected to the program
    constructor(path, callback) {
        console.log(path);
        this.arduino = new SerialPort({ path: path, baudRate: 115200 });
        this.callback = callback;
        this.parser = this.arduino.pipe(new ReadlineParser({ delimiter: '\r\n' }))
        this.parser.on('data', data => {
            console.log("received data")
            console.log(data);
            callback(data);
        });
    }

    writeJsonToArduino(json) {
        let buf = JSON.stringify(json);
        this.arduino.write(buf), function (err) {
            if (err) {
                return console.log("Error writing to arduino", err.message);
            }
        }
    }
}
module.exports = {
    PhysicalControls: PhysicalControls
};