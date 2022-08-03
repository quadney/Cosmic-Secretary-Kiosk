// const raspi = require('raspi');
// const Serial = require('raspi-serial').Serial;
const ephemeris = require('ephemeris');

const { SerialPort } = require('serialport');
const { PhysicalControls } = require('./PhysicalControls')

const fs = require('fs');
const config = JSON.parse(fs.readFileSync(`${__dirname}/public/config.json`));

const ARDUINO_COMMS = {
  ID: 0,
  MONTH: 1,
  DAY: 2,
  YEAR: 3,
  TIMEZONE: 4,
  LENGTH: 5
}

const locationDict = {
    "N-AM": {lat: 38.189003, long: -100.188901},
    "S-AM": {lat: -10.715016, long: -56.511485},
    "EURO": {lat: 47.947946, long: 2.292327},
    "AFRI": {lat: 7.297827, long: 23.255336},
    "AUST": {lat: -27.159117, long: 137.869196},
    "OCEA": {lat: -39.066017, long: 176.489426},
    "W-AS": {lat: 52.135676, long: 41.845175},
    "C-AS": {lat: 39.844309, long: 84.562250},
    "E-AS": {lat: 40.749300, long: 123.719569}
}

let arduinos = [];

let person1Birthday = undefined;
let person2Birthday = undefined;

SerialPort.list().then(ports => {
  console.log("checking serial ports");
  for (let port of ports) {
    console.log(port.path);
    if (port.path.includes('usb') || port.path.includes('tty')) {
      arduinos.push(new PhysicalControls(port.path, (data) => {
        console.log("callback function")
        console.log("setting data for arduino: "+ data);
        let parsedData = parseDataFromArduino(data);
        if (parsedData) {
          setBirthdayDataForArduino(parsedData);
          //TODO: 
        // update the other arduino for the data that we just made
        }
      }));
    }
  }
}).catch(err => {
  console.error(err);
});

function setBirthdayDataForArduino(birthdayData) {
  if (birthdayData.person == 0) {
    person1Birthday = birthdayData;
  }
  else if (birthdayData.person == 1) {
    person2Birthday = birthdayData;
  }
}

function parseDataFromArduino(data) {
  if (!data) {
    console.error("data is undefined");
    return;
  }

  if (data.includes(";")) {
    let arduinoId = data.split(";");
    return {person: arduinoId[0]};
  }

  let birthday = data.split("/");
  if(birthday.length !=ARDUINO_COMMS.LENGTH) {
    console.log("error = not enough data ");
    return;
  }
  console.log(birthday);
  let dateString = buildDateString(birthday[ARDUINO_COMMS.MONTH], birthday[ARDUINO_COMMS.DAY], birthday[ARDUINO_COMMS.YEAR]);
  let date = new Date(dateString);
  let location = locationDict[birthday[ARDUINO_COMMS.TIMEZONE]];
  let ephemeris_result = ephemeris.getAllPlanets(date, location.long, location.lat, 0);
  let results = burningManChartForPlanets(ephemeris_result);
  results.person = birthday[ARDUINO_COMMS.ID];
  return results;
}

function buildDateString(month, day, year) {
  let monthString = month;
  if (month < 10) {
    monthString = "0" + month;
  }

  let dayString = day;
  if (day < 10) {
    dayString = "0" + day;
  }

  return `${year}-${monthString}-${dayString}T00:00:00.000-00:00`;
}

function burningManChartForPlanets(result) {
  let chart = burningManChartDictionary();
  for (const key in chart) {
      if (chart.hasOwnProperty(key)) {
          let longitude = result.observed[key].apparentLongitudeDms360;
          let degree = longitude.split('°')[0];
          chart[key] = getElementForDegrees(degree);
      }
  }

  return chart;
}

function getElementForDegrees(deg) {
  let sign = config.signs.find(sign => {
      if (deg >= sign["min-deg"] && deg < sign["max-deg"]) {
          return sign;
      }
  });

  console.log(sign);
  return sign.element;
}

function burningManChartDictionary() {
  return {
      venus: undefined,
      moon: undefined,
      mercury: undefined
  }
}

function createChartForPlanets(result) {
  let chart = newChartDictionary();
  for (const key in chart) {
      if (chart.hasOwnProperty(key)) {
          let longitude = result.observed[key].apparentLongitudeDms360;
          let degree = longitude.split('°')[0];
          chart[key] = getSignForDegrees(degree);
      }
  }
  return chart;
}

function getSignForDegrees(deg) {
  let sign = config.signs.find(sign => {
      if (deg >= sign["min-deg"] && deg < sign["max-deg"]) {
          return sign;
      }
  });

  return sign.key;
}

function newChartDictionary() {
  return {
      sun: undefined,
      moon: undefined,
      mercury: undefined,
      venus: undefined,
      mars: undefined,
      jupiter: undefined,
      saturn: undefined,
      neptune: undefined,
      uranus: undefined,
      pluto: undefined
  }
}