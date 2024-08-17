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
"use strict";

let port;
let reader;
let inputDone;
let outputDone;
let inputStream;
let outputStream;

const log = document.getElementById("log");
const butConnect = document.getElementById("butConnect");
const butGetConfig = document.getElementById("butGetConfig");
const butDefault = document.getElementById("butDefault");
const butClearLog = document.getElementById("butClearLog");
const butSendConfig = document.getElementById("butSendConfig");
const butGetBTMac = document.getElementById("butGetBTMac");
const butSetBTAllow = document.getElementById("butSetBTAllow");
const butSaveConfig = document.getElementById("butSaveConfig");
toggleUIConnected(false);
const radioColor = document.getElementById("color_sensor");
const radioMatrix = document.getElementById("color_matrix");
const matrixSetup = document.getElementById("matrix-setup");

const field_neopixel_nrleds = document.getElementById("neopixel_nrleds");
const field_neopixel_gpio = document.getElementById("neopixel_gpio");
const field_bt_allow = document.getElementById("bt_allow");
const checkboxBTFilter = document.getElementById("bt_filter");
const field_bt_mac = document.getElementById("bt_mac");
// create array with fields 'color0, color1,' etc.
var color_fields = [];
for (var i = 0; i < 11; i++) {
    var color = document.getElementById(`color${i}`);
    color_fields.push(color);
}
// create array with fields 'map0, map1,' etc.
// var map_fields = [];
// for (var i = 1; i < 10; i++) {
//     var map = document.getElementById(`map${i}`);
//     map_fields.push(map);
// }

// some global variables for parsing configuration
var config = "";
var found_conf = 0;
var new_config = 0;
var parsedconfig = {};

document.addEventListener("DOMContentLoaded", () => {
    butConnect.addEventListener("click", clickConnect);
    butGetConfig.addEventListener("click", clickGetConfig);
    butDefault.addEventListener("click", clickDefault);
    butClearLog.addEventListener("click", clickClearLog);
    butGetBTMac.addEventListener("click", clickBTMac);
    butSetBTAllow.addEventListener("click", clickBTAllow);
    butClearLog.addEventListener("click", clickClearLog);
    
    butSendConfig.addEventListener("click", clickSendConfig);
    butSaveConfig.addEventListener("click", clickSaveConfig);
    radioColor.addEventListener("click", function(){matrixSetup.classList.remove("show");});
    radioMatrix.addEventListener("click", function(){matrixSetup.classList.add("show");});
    // const notSupported = document.getElementById('notSupported');
    // notSupported.classList.toggle('hidden', 'serial' in navigator);
});


// code van vincent
const gridWidth = 9; // X width
let gridHeight = 16; // Y height

field_neopixel_nrleds.value = gridHeight;
field_neopixel_nrleds.addEventListener("change", (event) => {
    gridHeight=field_neopixel_nrleds.value ;
    if (gridHeight>64) {gridHeight=64; field_neopixel_nrleds.value = gridHeight;}
    // console.log(`changed: ${gridHeight}`);
    updateGrid();
    SetMapping();
    updateSelectedCells();

  });

let controller_selected = [];
let controller_val=[];
let controller_bytes=[];
const gridContainer = document.getElementById('grid');

function updateGrid() {
    gridContainer.style.gridTemplateColumns = `repeat(${gridWidth+1}, 1fr)`;
    gridContainer.className = 'grid';
gridContainer.innerHTML = "";    
for (let col = -1; col < gridWidth; col++) {
  const cell = document.createElement('div');
  if(col == -1) {
      cell.className = 'cellnothing';
  } else {
      cell.className = 'cell_column_header';
      cell.innerHTML = col+1;
  }
  
  cell.dataset.row = -1;
  cell.dataset.col = col;
  
  gridContainer.appendChild(cell);
}
for (let row = 0; row < gridHeight; row++) {
  const rowcell = document.createElement('div');
  rowcell.className = 'cell_row_header';
  
  rowcell.dataset.row = -1;
  rowcell.dataset.col = row;
  rowcell.innerHTML = row;
  gridContainer.appendChild(rowcell);

  for (let col = 0; col < gridWidth; col++) {
    const cell = document.createElement('div');
    cell.className = 'cell';
    cell.dataset.row = row;
    cell.dataset.col = col;
    cell.addEventListener('click', function() {
      selectCell(row, col);
    });
    gridContainer.appendChild(cell);
    //if(col == 0) {
    //  setCellSelected(row, col, true);
   // }
  }
}
}
updateGrid();
SetMapping();
updateSelectedCells();

function cellIsSelected(row, col) {
  const selectedCell = document.querySelector(`.cell[data-row='${row}'][data-col='${col}']`);
  return selectedCell.classList.contains('selected');
}

function updateSelectedCellsText() {
  //const gridData = document.getElementById('griddata');
  //gridData.innerHTML = '';
  controller_bytes=[];
  for(let col=0;col<gridWidth;col++) {
      let bytes=[0,0,0,0,0,0,0,0];
      for (let b=0; b<8; b++) {
         for (let bit=0; bit<8; bit++) {
            if (controller_selected[col].includes(b*8+bit)) {
                bytes[b]+=(1<<bit);
            }
         }
      }
      controller_bytes.push(bytes);
   //   gridData.innerHTML += `controller ${col}: ${controller_selected[col].join(', ')}  bytes: ${bytes.join(', ')}<br>`;
  }

}

function updateSelectedCells() {
  controller_selected = [];

  for(let col=0;col<gridWidth;col++) {
      controller_selected[col] = [];
      for(let row=0;row<gridHeight;row++) {
          if(cellIsSelected(row, col)) {
              controller_selected[col].push(row);
          }
      }
  }
  updateSelectedCellsText();
}

function setCellSelected(row, col, selected) {
  const cell = document.querySelector(`.cell[data-row='${row}'][data-col='${col}']`);
  if (selected) {
    cell.classList.add('selected');
  } else {
    cell.classList.remove('selected');
  }
}

function selectCell(row, col) {
    if ( cellIsSelected(row,col)) { // already selected, just deselect it
        setCellSelected(row,col,false);
    } else {
        const allCellsInRow = document.querySelectorAll(`.cell[data-row='${row}']`);
        allCellsInRow.forEach(cell => cell.classList.remove('selected'));
        setCellSelected(row, col, true);
    }
  updateSelectedCells();
}

function SetMapping() {
    // console.log(`set mapping ${gridHeight}`);
    for (let i=0; i<9; i++) {
    let bytes=controller_bytes[i];
    for (let b=0; b<8; b++) {
        let bb=bytes[b];
        for (let bit=0; bit<8; bit++) {
            if (b*8+bit<gridHeight) {
                //if (bb&1) {
                    setCellSelected(b*8+bit,i,bb&1);
                //}
            } //else {
               // console.log(`${b*8+bit} > ${gridHeight}`);
           // }
             bb>>=1;
        }
     }
    }
     updateSelectedCells();

}
// end code vincent


async function connect() {
    const filter = {
        usbVendorId: 0x1a86, // CH340
        usbProductId: 0x7523,
    };
    // - Request a port and open a connection.
    port = await navigator.serial.requestPort({ filters: [filter] ,bufferSize:10000});
    // console.log(port);
    // - Wait for the port to open.
    await port.open({ baudRate: 115200, bufferSize: 10000, flowControl:"none" });

    const encoder = new TextEncoderStream();
    outputDone = encoder.readable.pipeTo(port.writable);
    outputStream = encoder.writable;
    writeToStream("response 0"); // select non responsive interface on LMS-ESP32
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
    var redHex = r.toString(16).padStart(2, "0");
    var greenHex = g.toString(16).padStart(2, "0");
    var blueHex = b.toString(16).padStart(2, "0");

    // Concatenate the hexadecimal values
    var hexColor = "#" + redHex + greenHex + blueHex;

    return hexColor;
}

function hexToRgb(hex) {
    // Remove '#' if it's present
    hex = hex.replace(/^#/, "");

    // Convert hex to RGB
    var r = parseInt(hex.substring(0, 2), 16);
    var g = parseInt(hex.substring(2, 4), 16);
    var b = parseInt(hex.substring(4, 6), 16);

    return { r: r, g: g, b: b };
}

function parseconfig(a) {
    // console.log("=====================\n"+a+"==============================");
    var lines = a.split("\n");
    var l = lines.length;
    function getnums(a) {
        var b = a.map(function (item) {
            return parseInt(item, 10);
        });
        return b;
    }
    // magic
    var sensor_id = parseInt(lines[1].split(" ").pop());
    //console.log(sensor_id);
    var neopixel_nrleds = parseInt(lines[2].split(" ").pop());
    //console.log(neopixel_nrleds);
    var neopixel_gpio = parseInt(lines[3].split(" ").pop());
    //console.log(neopixel_gpio);
    var mapping = [];
    for (var j = 0; j < 9; j++) {
        var map = lines[j + 5].split(" ");
        map.shift();
        map.pop();
        var mapnums = getnums(map);
        mapping.push(mapnums);
    }
    // console.log('MAPNUMS');
    // console.log(mapping);
    //try{
    var colors = [];
    for (var j = 0; j < 11; j++) {
        var col = lines[j + 14].split(" ");
        col.shift();
        col.pop();
        var colnums = getnums(col);
        colors.push(colnums);
    }
    // console.log(colors);
    var bt_filter = parseInt(lines[25].split(" ").pop());
    var bt_allow="";
    var mac_bytes = lines[26].split(" ");
    mac_bytes.shift();
    mac_bytes.pop();
    mac_bytes=getnums(mac_bytes);
    for (var i=0; i< 6; i++ ) {
        var s = mac_bytes[i].toString(16)
        if(s.length < 2) {
            s = '0' + s;
        }
        bt_allow+=s+':'
    }   
    bt_allow=bt_allow.slice(0, -1); 
    var bt_mac="";
    var mac_bytes = lines[27].split(" ");
    mac_bytes.shift();
    mac_bytes.pop();
    mac_bytes=getnums(mac_bytes);
    for (var i=0; i< 6; i++ ) {
        var s = mac_bytes[i].toString(16)
        if(s.length < 2) {
            s = '0' + s;
        }
        bt_mac+=s+':'
    }   
    bt_mac=bt_mac.slice(0, -1); 
    
    return {
        sensor_id: sensor_id,
        neopixel_nrleds: neopixel_nrleds,
        neopixel_gpio: neopixel_gpio,
        mapping: mapping,
        colors: colors,
        bt_filter: bt_filter,
        bt_allow: bt_allow,
        bt_mac: bt_mac
    };
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
async function clickClearNeopixel() {
    writeToStream(`neopixel clear`);
}

async function clickSetNeopixel() {
    var np_pixel = neopixel_pixel.value;
    var c = hexToRgb(neopixel_color.value);
    writeToStream(`neopixel set ${np_pixel} ${c.r} ${c.g} ${c.b}`);
}

async function clickLegoColor() {
    const lego_colors = [
        "#000000",
        "#C8C8FF",
        "#FF00FF",
        "#0000FF",
        "#00FFFF",
        "#00FF96",
        "#00FF00",
        "#FFFF00",
        "#FF8C00",
        "#FF0000",
        "#FFFFFF",
    ];
    for (var i = 0; i < 11; i++) {
        color_fields[i].value = lego_colors[i];
    }
}

async function clickSendLegoColor(){
    var sensor_id = 0;
    if (radioColor.checked) {
        sensor_id = 61;
    } else {
        sensor_id = 64;
    }
    if (sensor_id == 64) {
        writeToStream("response 0");
        for (var i = 0; i < 11; i++) {
            var c = hexToRgb(color_fields[i].value);
            //console.log(c)
            writeToStream(`set color ${i} ${c.r} ${c.g} ${c.b}`);
        }
    }
}

async function clickDefaultMapping() {
    // for (var i = 0; i < 9; i++) {
    //     map_fields[i].value = i;
    // }
    controller_bytes=[];
    controller_bytes.push([1,0,0,0]);
    controller_bytes.push([2,0,0,0]);
    controller_bytes.push([4,0,0,0]);
    controller_bytes.push([8,0,0,0]);
    controller_bytes.push([16,0,0,0]);
    controller_bytes.push([32,0,0,0]);
    controller_bytes.push([64,0,0,0]);
    controller_bytes.push([128,0,0,0]);
    controller_bytes.push([0,1,0,0]);
    SetMapping();
    updateSelectedCells();

}

async function clickDefault() {
    writeToStream("default");
    writeToStream("eeprom clear");
    writeToStream("show");
}

async function clickClearLog() {
    log.textContent = "";
}

async function clickGetConfig() {
    // console.log("clickGetConfig");
    config = "";
    writeToStream("show", "");
}

async function clickSaveConfig() {
    clickSendConfig();
    writeToStream(`save`);
}

async function clickBTAllow() {
    clickSendConfig();
    writeToStream(`save`);
}

async function clickBTMac() {
    config = "";
    writeToStream("show", "");
}


async function clickSendMapping() {
    writeToStream("response 0"); // select non responsive interface on LMS-ESP32
    var sensor_id = 0;
    if (radioColor.checked) {
        sensor_id = 61;
    } else {
        sensor_id = 64;
    }
    writeToStream(`set sensor ${sensor_id}`);
    if (sensor_id == 64) {
        // only for color_matrix
        var neopixel_nrleds = field_neopixel_nrleds.value;
        var neopixel_gpio = field_neopixel_gpio.value;
        writeToStream(`set np_nr ${neopixel_nrleds}`);
        writeToStream(`set np_gpio ${neopixel_gpio}`);

        for (var i = 0; i < 9; i++) {
            writeToStream(`set map ${i} ${controller_bytes[i].join(' ')}`);
            //for (let kk=0; kk<100000000; kk++) {let a=1;}
            // console.log(`next ${i}`)
        }
    }

}
async function clickSendConfig() {
    writeToStream("response 0"); // select non responsive interface on LMS-ESP32
    
    // console.log("clickSendConfig");
    // collect information from webpage
    var sensor_id = 0;
    if (radioColor.checked) {
        sensor_id = 61;
    } else {
        sensor_id = 64;
    }
    writeToStream(`set sensor ${sensor_id}`);
    if (sensor_id == 64) {
        // only for color_matrix
        var neopixel_nrleds = field_neopixel_nrleds.value;
        var neopixel_gpio = field_neopixel_gpio.value;
        writeToStream(`set np_nr ${neopixel_nrleds}`);
        writeToStream(`set np_gpio ${neopixel_gpio}`);

        for (var i = 0; i < 9; i++) {
            writeToStream(`set map ${i} ${controller_bytes[i].join(' ')}`);
            //for (let kk=0; kk<100000000; kk++) {let a=1;}
            // console.log(`next ${i}`)
        }
        for (var i = 0; i < 11; i++) {
            var c = hexToRgb(color_fields[i].value);
            //console.log(c)
            writeToStream(`set color ${i} ${c.r} ${c.g} ${c.b}`);
        }

    }
    var bt_filter=0;
    if (checkboxBTFilter.checked) bt_filter=1;
    writeToStream(`set bt_filter ${bt_filter}`);
    var mac_address = field_bt_mac.value;
    field_bt_allow.value=mac_address;
    var mac_bytes=mac_address.split(':');
    // console.log("mac_bytes:"+mac_bytes);
    var mac_arr=[];
    for (var i=0; i<6; i++ ) {
        mac_arr.push( parseInt(mac_bytes[i], 16));
    }
    writeToStream(`set bt_allow ${mac_arr[0]} ${mac_arr[1]} ${mac_arr[2]} ${mac_arr[3]} ${mac_arr[4]} ${mac_arr[5]}`);
}

/**
 * @name readLoop
 * Reads data from the input stream and displays it on screen.
 */
async function readLoop() {
    // CODELAB: Add read loop here.
    while (true) {
        const { value, done } = await reader.read();
        if (value) {
            // console.log("value=",value);
            new_config = 0;

            config = config + value;
            var start_magic = config.substring(0,10).indexOf("magic");
            if (found_conf==0 && start_magic >= 0) {
                found_conf = 1;
                // console.log("magic found");
                var start_magic = config.indexOf("magic");
                config = config.slice(start_magic);
            }
            if (found_conf == 1 && config.indexOf("OK") > 0) {
                // console.log("OK found");
                new_config = 1;
                found_conf = 0;
            }
            if (new_config == 1) {
                new_config = 0;
                var start_magic = config.indexOf("magic");
                var OK = config.indexOf("OK");
                config = config.slice(start_magic);
                parsedconfig = parseconfig(config);
                // console.log(parsedconfig);
                config = "";
                if (parsedconfig.sensor_id == 64) {
                    radioMatrix.checked = true;
                    matrixSetup.classList.add("show");
                } else if (parsedconfig.sensor_id == 61) {
                    radioColor.checked = true;
                    matrixSetup.classList.remove("show");
                }
                field_neopixel_nrleds.value = parsedconfig.neopixel_nrleds;
                field_neopixel_gpio.value = parsedconfig.neopixel_gpio;
                gridHeight=parsedconfig.neopixel_nrleds;
                updateGrid();
                for (var i = 0; i < 9; i++) {
                    let map= parsedconfig.mapping[i];
                    controller_bytes[i]=map; // skip first item
                    for (let b=0; b<8; b++) {
                      let bb=map[b];
                      for (let bit=0; bit<8; bit++) {
                        if (b*8+bit<gridHeight) {
                           //if (bb&1) {
                                setCellSelected(b*8+bit,i,b&1);
                           // }
                        }
                         bb>>=1;
                      }
                    }
                }
                
                SetMapping();
                updateSelectedCells();
                
                for (var i = 0; i < 11; i++) {
                    var cc = parsedconfig.colors[i];
                    var r, g, b;
                    r = cc[1];
                    g = cc[2];
                    b = cc[3];
                    var colcode = rgbToHex(r, g, b);
                    color_fields[i].value = colcode;
                }
                new_config = 0;
                field_bt_allow.value = parsedconfig.bt_allow;
                field_bt_mac.value = parsedconfig.bt_mac;
                
                checkboxBTFilter.value = parsedconfig.bt_filter;

               // console.log(value);
            }
            log.textContent = log.textContent + value;
            log.scrollTop = log.scrollHeight;
       
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
        // console.log("[SEND]", line);
        writer.write(line + "\r");
        //let delayres =  delay(100);
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
        this.container = "";
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
        cb.addEventListener("change", () => {
            sendGrid();
        });
    });
}

function toggleUIConnected(connected) {
    let lbl = "Connect";
    if (connected) {
        lbl = "Disconnect";
    }
    butConnect.textContent = lbl;
    if (connected) {
        butGetConfig.removeAttribute("disabled");
        butDefault.removeAttribute("disabled");
        butClearLog.removeAttribute("disabled");
        butSendConfig.removeAttribute("disabled");
        butSaveConfig.removeAttribute("disabled");
        butGetBTMac.removeAttribute("disabled");
        butSetBTAllow.removeAttribute("disabled");
        
    } else {
        butGetConfig.setAttribute("disabled", true);
        butDefault.setAttribute("disabled", true);
        butClearLog.setAttribute("disabled", true);
        butSendConfig.setAttribute("disabled", true);
        butSaveConfig.setAttribute("disabled", true);
        butGetBTMac.setAttribute("disabled", true);
        butSetBTAllow.setAttribute("disabled", true);
        //   cb.setAttribute('disabled', true);
    }
}
