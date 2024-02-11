/*
 * @license
 * Getting Started with Web Serial Codelab (https://todo)
 * Copyright 2019 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */
'use strict';

let port;
let reader;
let inputDone;
let outputDone;
let inputStream;
let outputStream;

const log = document.getElementById('log');
const butConnect = document.getElementById('butConnect');
const butGetConfig = document.getElementById('butGetConfig');
const butDefault = document.getElementById('butDefault');
const butClearLog = document.getElementById('butClearLog');
const butSendConfig = document.getElementById('butSendConfig');
const butSaveConfig = document.getElementById('butSaveConfig');

const radioColor = document.getElementById('color_sensor');
const radioMatrix = document.getElementById('color_matrix');

const field_neopixel_nrleds = document.getElementById('neopixel_nrleds');
const field_neopixel_gpio = document.getElementById('neopixel_gpio');
// create array with fields 'color0, color1,' etc.
var color_fields=[];
for (var i=0; i<11; i++) {
  var color = document.getElementById(`color${i}`);
  color_fields.push(color);
}
// create array with fields 'map0, map1,' etc.
var map_fields=[]
for (var i=1; i<10; i++) {
  var map = document.getElementById(`map${i}`);
  map_fields.push(map);
}

// some global variables for parsing configuration
var config="";
var found_conf=0;
var new_config=0;
var parsedconfig={};

document.addEventListener('DOMContentLoaded', () => {
  butConnect.addEventListener('click', clickConnect);
  butGetConfig.addEventListener('click', clickGetConfig);
  butDefault.addEventListener('click', clickDefault);
  butClearLog.addEventListener('click', clickClearLog);
  butSendConfig.addEventListener('click', clickSendConfig);
  butSaveConfig.addEventListener('click', clickSaveConfig);
  // const notSupported = document.getElementById('notSupported');
  // notSupported.classList.toggle('hidden', 'serial' in navigator);
});


async function connect() {

const filter = {
	usbVendorId: 0x1a86, // CH340
	usbProductId: 0x7523
};
// - Request a port and open a connection.
port = await navigator.serial.requestPort({filters: [filter]});
console.log(port);
// - Wait for the port to open.
await port.open({ baudRate: 115200, bufferSize: 100000 });

  const encoder = new TextEncoderStream();
  outputDone = encoder.readable.pipeTo(port.writable);
  outputStream = encoder.writable;
  writeToStream('response 0'); // select non responsive interface on LMS-ESP32
  writeToStream("show");
  writeToStream("show");

  let decoder = new TextDecoderStream();
  inputDone = port.readable.pipeTo(decoder.writable);
  inputStream = decoder.readable;

  reader = inputStream.getReader();
  readLoop();
  writeToStream("show");
}

function rgbToHex(r, g, b) {
  // Convert each RGB component to hexadecimal representation
  var redHex = r.toString(16).padStart(2, '0');
  var greenHex = g.toString(16).padStart(2, '0');
  var blueHex = b.toString(16).padStart(2, '0');

  // Concatenate the hexadecimal values
  var hexColor = '#' + redHex + greenHex + blueHex;

  return hexColor;
}

function hexToRgb(hex) {
  // Remove '#' if it's present
  hex = hex.replace(/^#/, '');

  // Convert hex to RGB
  var r = parseInt(hex.substring(0, 2), 16);
  var g = parseInt(hex.substring(2, 4), 16);
  var b = parseInt(hex.substring(4, 6), 16);

  return { r: r, g: g, b: b };
}



function parseconfig(a) {
  var lines=a.split('\n');
  var l=lines.length;
  function getnums(a){
     var b = a.map(function(item) {
            return parseInt(item, 10);
           });
     return b;
  }
  // magic
  var sensor_id=parseInt(lines[1].split(' ').pop())
  //console.log(sensor_id);
  var neopixel_nrleds=parseInt(lines[2].split(' ').pop())
  //console.log(neopixel_nrleds);
  var neopixel_gpio=parseInt(lines[3].split(' ').pop())
  //console.log(neopixel_gpio);
  var mapping = lines[4].split(' ');
  mapping.shift(); // remove first element
  mapping.pop(); // remoce last element
  var mapnums=getnums(mapping);
  //console.log(mapnums);
  //try{
  var colors=[]
  for (var j=0; j<11; j++) {
    var col=lines[j+5].split(' ');
    col.shift()
    col.pop()
    var colnums=getnums(col);
    colors.push(colnums);
  }
  //console.log(colors);

  return {'sensor_id':sensor_id,'neopixel_nrleds':neopixel_nrleds,'neopixel_gpio':neopixel_gpio,
  'mapnums':mapnums,'colors':colors};
} //catch {console.log("error");}

async function disconnect() {
  if (reader) {
    await reader.cancel();
    await inputDone.catch(() => {});
    reader = null;
    inputDone = null;
  }
  if (outputStream) {
    await outputStream.getWriter().close();
    await outputDone;
    outputStream = null;
    outputDone = null;
  }
  await port.close();
  port = null;
}

async function clickConnect() {
  // CODELAB: Add disconnect code here.
  if (port) {
    await disconnect();
    toggleUIConnected(false);
    return;
  }
  toggleUIConnected(true);
  
  await connect();
}


async function clickLegoColor() {
  const lego_colors= ['#000000', '#C8C8FF', '#FF00FF', '#0000FF',
                      '#00FFFF', '#00FF96', '#00FF00', '#FFFF00',
                      '#FF8C00','#FF0000', '#FFFFFF'];
  for (var i=0; i<11; i++){
    color_fields[i].value = lego_colors[i];
  }
}

async function clickDefaultMapping() {
  for (var i=0; i<9; i++) {
    map_fields[i].value = i;
  }
}


async function clickDefault() {
  writeToStream("default");
  writeToStream("show");
}

async function clickClearLog() {
  log.textContent= "";
}

async function clickGetConfig() {
  config="";
  writeToStream("show","");
}

async function clickSaveConfig() {
  writeToStream(`save`);
}

async function clickSendConfig() {
  // collect information from webpage
  var sensor_id=0;
  if (radioColor.checked) {
    sensor_id=61;
  } else {
    sensor_id=64;
  }
  writeToStream(`set sensor ${sensor_id}`);
  if (sensor_id==64) { // only for color_matrix
    var neopixel_nrleds=field_neopixel_nrleds.value;
    var neopixel_gpio=field_neopixel_gpio.value;
    writeToStream(`set np_nr ${neopixel_nrleds}`);
    writeToStream(`set np_gpio ${neopixel_gpio}`);

    for (var i=0; i<11; i++) {
      var c=hexToRgb(color_fields[i].value);
      console.log(c)
      writeToStream(`set color ${i} ${c.r} ${c.g} ${c.b}` );
    }
    for (var i=0; i<9; i++) {
      writeToStream(`set map ${i} ${map_fields[i].value}`);
    }
  }
}

/**
 * @name readLoop
 * Reads data from the input stream and displays it on screen.
 */
async function readLoop() {
  // CODELAB: Add read loop here.
// CODELAB: Add read loop here.
while (true) {
  const { value, done } = await reader.read();
  if (value) {
    var start_magic=value.indexOf('magic');
    if (start_magic>=0) {
      found_conf=1;
    
    } 
    config=config+value;  
    if (found_conf==1 && value.indexOf("OK")>=0) {
      new_config=1;
      found_conf=0;
    }
    if (new_config==1) {
      
      var start_magic=config.indexOf('magic');
      var OK=config.indexOf('OK');
      config=config.slice(start_magic);
      parsedconfig=parseconfig(config);
      console.log(parsedconfig);
      config="";
      if (parsedconfig.sensor_id==64) {
        radioMatrix.checked=true;
      } else
      if (parsedconfig.sensor_id==61) {
        radioColor.checked=true;
      }
      field_neopixel_nrleds.value = parsedconfig.neopixel_nrleds;
      field_neopixel_gpio.value = parsedconfig.neopixel_gpio;
      for (var i=0; i<9; i++) {
        map_fields[i].value=parsedconfig.mapnums[i];
      }
      for (var i=0; i<11; i++) {
        var cc=parsedconfig.colors[i];
        var r,g,b;
        r=cc[1]; g=cc[2]; b=cc[3];
        var colcode=rgbToHex(r,g,b);
        color_fields[i].value=colcode;
      }
      new_config=0;
    }
    log.textContent = log.textContent + value;  
    // console.log(value);
  }
  
  if (done) {
    // console.log('[readLoop] DONE', done);
    reader.releaseLock();
    break;
  }
}
}




/**
 * @name writeToStream
 * Gets a writer from the output stream and send the lines to the micro:bit.
 * @param  {...string} lines lines to send to the micro:bit
 */
function writeToStream(...lines) {
  // CODELAB: Write to output stream
// CODELAB: Write to output stream
const writer = outputStream.getWriter();
lines.forEach((line) => {
  console.log('[SEND]', line);
  writer.write(line + '\r');
});
writer.releaseLock();
}


/**
 * @name watchButton
 * Tells the micro:bit to print a string on the console on button press.
 * @param {String} btnId Button ID (either BTN1 or BTN2)
 */
function watchButton(btnId) {
  // CODELAB: Hook up the micro:bit buttons to print a string.

}


/**
 * @name LineBreakTransformer
 * TransformStream to parse the stream into lines.
 */
class LineBreakTransformer {
  constructor() {
    // A container for holding stream data until a new line.
    this.container = '';
  }

  transform(chunk, controller) {
    // CODELAB: Handle incoming chunk

  }

  flush(controller) {
    // CODELAB: Flush the stream.

  }
}


/**
 * @name JSONTransformer
 * TransformStream to parse the stream into a JSON object.
 */
class JSONTransformer {
  transform(chunk, controller) {
    // CODELAB: Attempt to parse JSON content

  }
}


/**
 * @name buttonPushed
 * Event handler called when one of the micro:bit buttons is pushed.
 * @param {Object} butEvt
 */
function buttonPushed(butEvt) {
  // CODELAB: micro:bit button press handler

}


/**
 * The code below is mostly UI code and is provided to simplify the codelab.
 */

function initCheckboxes() {
  ledCBs.forEach((cb) => {
    cb.addEventListener('change', () => {
      sendGrid();
    });
  });
}


function toggleUIConnected(connected) {
  let lbl = 'Connect';
  if (connected) {
    lbl = 'Disconnect';
  }
  butConnect.textContent = lbl;
  // ledCBs.forEach((cb) => {
  //   if (connected) {
  //     cb.removeAttribute('disabled');
  //     return;
  //   }
  //   cb.setAttribute('disabled', true);
  // });
}
