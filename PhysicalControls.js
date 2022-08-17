const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline')

class PhysicalControls {
    // one PhysicalControls object per each arduino connected to the program
    constructor(path, callback) {
        console.log(path);
        this.arduino = new SerialPort({ path: path, baudRate: 115200 });
        this.pinging = false;
        this.lastPing = -1;
        this.id = -1;
        this.callback = callback;
        this.parser = this.arduino.pipe(new ReadlineParser({ delimiter: '\r\n' }))
        this.parser.on('data', data => {
            if (this.pinging) {
                if (data["T"] == "A") {
                    this.pinging = false;
                    this.lastPing = Date.now();
                }
            }
            else if (data["T"] == "B") {
                this.id = parseInt(data["PL"]);
            }
            else {
                callback(data);
            }
        });

        // this.pingInterval = setInterval();
    }

    getId() {
        let data = { "T" : "B" };
        let buf = JSON.stringify(data);
        this.arduino.write((buf), function (err) {
            if (err) {
                return console.log("Error getting id from arduino ", err.message);
            }
        })
    }

    ping() {
        this.pinging = true;
        let data = { "T": "P" };
        let buf = JSON.stringify(data);
        this.arduino.write((buf), function (err) {
            if (err) {
                return console.log("Error pinging arduino ", err.message);
            }
        })
    }

    writeJsonToArduino(json) {
        let data = {"T" : "D", "PL": json};
        let buf = JSON.stringify(data);
        this.arduino.write((buf), function (err) {
            if (err) {
                return console.log("Error writing to arduino", err.message);
            }
        });
    }
}
module.exports = {
    PhysicalControls: PhysicalControls
};