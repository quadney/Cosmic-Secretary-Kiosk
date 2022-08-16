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

const MEDALLION_INDEX = {
  INTUITION: 0,
  LOVE: 1,
  COMMUNICATION: 2
}

const ELEMENT_INDEX = {
  EARTH: 1,
  AIR: 2, 
  WATER: 3,
  FIRE: 4,
  COMPATABLE: 5,
  DARK: 9,
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
  for (let port of ports) {
    if (port.path.includes('usb') || port.path.includes('tty') && !port.path.includes('ttyAMA0') && !port.path.includes('ttyS0')) {
      arduinos.push(new PhysicalControls(port.path, (data) => {
        let parsedData = parseDataFromArduino(data);
        if (parsedData) {
          setBirthdayDataForArduino(parsedData);
          //TODO: 
          // update the other arduino for the data that we just made
          sendDataToLEDArduino();
        }
      }));
    }
  }
}).catch(err => {
  console.error(err);
});

function sendDataToLEDArduino() {
  // { venus: 'fire', moon: 'air', mercury: 'earth', person: '0' }
  let payload;
  if (person1Birthday && person2Birthday) {
    // if both people have their hand on the thing, show what they have in common
    // check each value and see how they compare 
    payload = arduinoDataForPersons(person1Birthday, person2Birthday);
  }
  else if (person1Birthday) {
    payload = arduinoDataForPerson(person1Birthday);
  }
  else if (person2Birthday) {
    payload = arduinoDataForPerson(person2Birthday);
  }

  let json = {L : payload};
  console.log("sending json: ", json);
  arduinos.forEach(arduino => {
    arduino.writeJsonToArduino(json);
  })
}

function arduinoDataForPersons(person1, person2) {
  // might just be easier to do them all one by one 
  // venus
  let intuition = isCompatable(person1.moon, person2.moon) ? ELEMENT_INDEX.COMPATABLE : ELEMENT_INDEX.DARK;
  let love = isCompatable(person1.venus, person2.venus) ? ELEMENT_INDEX.COMPATABLE : ELEMENT_INDEX.DARK;
  let communication = isCompatable(person1.mercury, person2.mercury) ? ELEMENT_INDEX.COMPATABLE : ELEMENT_INDEX.DARK;

  return "" + intuition + "" + love + "" + communication;
}

function isCompatable(element1, element2) {
  if (element1 == "fire" || element1 == "air") {
    if (element2 == "air" || element2 == "fire") {
      return true;
    }
  }
  else if (element1 == "water" || element1 == "earth") {
    if (element2 == "water" || element2 == "earth") {
      return true;
    }
  }

  return false;
}

function arduinoDataForPerson(personData) {
  //{ venus: 'fire', moon: 'air', mercury: 'earth', person: '0' }
  let intuitionIndex = indexForElement(personData.moon);
  let loveIndex = indexForElement(personData.venus);
  let communicationIndex = indexForElement(personData.mercury);

  return intuitionIndex + "" + loveIndex + "" + communicationIndex;
}

function indexForElement(element) {
  if (element == "fire") {
    return ELEMENT_INDEX.FIRE;
  }
  else if (element == "water") {
    return ELEMENT_INDEX.WATER;
  }
  else if (element == "air") {
    return ELEMENT_INDEX.AIR;
  }
  else if (element == "earth") {
    return ELEMENT_INDEX.EARTH;
  }
  else {
    return ELEMENT_INDEX.DARK;
  }
}

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
    return;
  }

  if (data.includes(";")) {
    let arduinoId = data.split(";");
    return {person: arduinoId[0]};
  }

  let birthday = data.split("/");
  if(birthday.length != ARDUINO_COMMS.LENGTH) {
    return;
  }
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