import * as http from 'http';
import express from 'express';
import ip from 'ip';
import path from 'path';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import WebSocket, { WebSocketServer } from 'ws';

const __dirname = dirname(fileURLToPath(import.meta.url));

const app = express();

const hostname = '192.168.151.144';
const port = 80;

let lastLightState = false;

const server = http.createServer(app).listen(port);

const wss1 = new WebSocketServer({ port: 8888 }); // WebSocket server for browser clients
const wss2 = new WebSocketServer({ port: 8811 }); // WebSocket server for ESP32 modules


app.get('/', function (req, res) {
    res.sendFile(path.join(__dirname + '/fotoled3.html'));
});

wss1.broadcast = function broadcast(data) {
    wss1.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
            client.send(data);
        }
    });
};

wss2.broadcast = function broadcast(data) {
    wss2.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
            client.send(data);
        }
    });
};

wss1.on('connection', function (ws, req) {
    ws.on('message', function (strMessage) {
       
        const msg = JSON.parse(strMessage);
        if (msg.tipSporočila === "LED" || msg.tipSporočila === "tipka") {
            funkcijaLED(msg, ws);
        } else if(msg.tipSporočila = "Svetloba") {
            funkcijaFotoupornik(msg)
        }
        else if(msg.tipSporočila = "potenciometer") {
            funkcijaPotenciometer(msg)
    }
    });
});

/*wss2.on('connection', function(ws, req){
    console.log("Vzpostavljena je povezava z ESP na 8811 wss2");  
    ws.on('message', function(strMessage){        
        var msg=JSON.parse(strMessage);
        console.log(msg.tipSporočila + " wss2 message");
        switch(msg.tipSporočila){
            case "potenciometer":
                funkcijaPotenciometer(msg);                
                break; 
            case "LED":
                funkcijaLEDwss2(msg);
                console.log("Sporočilo sprejeto na wss2 LED");                
            break; 
            case "Svetloba":
                funkcijaFotoupornik(msg);                
                break;               
        }
    });   
});*/

// Handling WebSocket connections na ESP32 8811
wss2.on('connection', (ws, req) => {
    console.log("ESP32 se je povezal z dvosmerno povezavo prek vrat 8811.");
    ws.on("message", (msgString) => { // Handle incoming messages
        console.log("Sporočilo prejeto, vsebina: " + msgString);
 
        var msg = JSON.parse(msgString); //msg je sedaj JSON objekt, s funkcijo parse razčlenimo strin
        switch(msg.tipSporočila){
            case "potenciometer" :
                funkcijaPotenciometer(msg); //tip sporočila iz Chrome poženemo funkcijo "potenciometer"
                console.log("Potenciometer pošilja podatke.");
                break;
        }
    }); // End of ws.on("message")
   }); // End of wss2.on('connection')
 
console.log('Strežnik zagnan');
console.log('IP=' + ip.address()); // Log server IP address
 
function funkcijaPotenciometer(msg){ // 
    wss1.broadcast(JSON.stringify(msg)); //broadcastamo na vseh chrome clientih
}



function funkcijaLED(msg, ws) {
    if (msg.pin === "5" && msg.status === "pressed") {
        const ledMsg = JSON.stringify({ tipSporočila: "LED", pin: ["PIN5"], vrednost: 1 });
        wss2.broadcast(ledMsg);
    } else {
        wss1.broadcast(JSON.stringify(msg));
        wss2.broadcast(JSON.stringify(msg));
    }
}

function funkcijaFotoupornik(message) {
  
            const lightState = message.stanje === 1 ? true : false;
            if (lightState !== lastLightState) {
                console.log(`Sprememba svetlobnih pogojev: ${lightState ? 'prisotna' : 'ni prisotna'}`);
                const ledStateMsg = JSON.stringify({ tipSporočila: "Svetloba", pin: "PIN5", vrednost: lightState ? 1 : 0 });
                wss1.broadcast(ledStateMsg);
                lastLightState = lightState;
            }


        
  
}


console.log("Server is running at http://" + ip.address() + ":" + port);
