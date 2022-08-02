const raspi = require('raspi');
const Serial = require('raspi-serial').Serial;
const ephemeris = require('ephemeris');
const moment = require('moment');

const fs = require('fs');
const config = JSON.parse(fs.readFileSync(`${__dirname}/public/config.json`));

const locationDict = {
    "north-america": {lat: 38.189003, long: -100.188901},
    "south-america": {lat: -10.715016, long: -56.511485},
    "europe": {lat: 47.947946, long: 2.292327},
    "africa": {lat: 7.297827, long: 23.255336},
    "australia": {lat: -27.159117, long: 137.869196},
    "oceania": {lat: -39.066017, long: 176.489426},
    "west-asia": {lat: 52.135676, long: 41.845175},
    "central-asia": {lat: 39.844309, long: 84.562250},
    "east-asia": {lat: 40.749300, long: 123.719569}
}

raspi.init(() => {
  var serial = new Serial();
  serial.open(() => {
    serial.on('data', (data) => {
      process.stdout.write(data);
    });
    serial.write('Hello from raspi-serial');
  });
});


function resultsForPerson(month, day, year, region) {
  let date = new Date()
  let location = locationDict[region];
  let ephemeris_result = ephemeris.getAllPlanets(date, location.long, location.lat, 0);
  let results = burningManChartForPlanets(ephemeris_result);
  console.log(results);
  return results;
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