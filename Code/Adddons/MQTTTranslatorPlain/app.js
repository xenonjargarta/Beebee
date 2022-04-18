const { Console } = require('console');
var mqtt = require('mqtt');
var count =0;
var client  = mqtt.connect("mqtt://digitalbeekeeper.de",{clientId:"translator"});

var message="test message";

var topic="hives/plain";
console.log("subscribing to topics");
client.subscribe(topic,{qos:1}); //single topic


//var timer_id=setInterval(function(){publish(topic,message,options);},5000);

//notice this is printed even before we connect
console.log("connected flag  " + client.connected);

//handle incoming messages
client.on('message',function(topic, message, packet)
{
	console.log("message is "+ message);
	console.log("topic is "+ topic);
    var myArray = message.toString().split(";");
    console.log("message has "+ myArray.length + " parts");
    if(myArray.length == 8)
    {
        console.log("Message is correct");
        //publish Temperature (DS18)
        if(myArray[2].trim() != "")
        {
            var vibarray = myArray[2].trim().toString().split("&");
            if(vibarray.length == 3)
            {
                publish(myArray[0].trim() + "/vibx^x", vibarray[0].trim(), options);
                publish(myArray[0].trim() + "/vibyy", vibarray[1].trim(), options);
                publish(myArray[0].trim() + "/vibz", vibarray[2].trim(), options);
            }
        }
        if(myArray[3].trim() != "")
        {
            publish(myArray[0].trim() + "/temp", myArray[3].trim(), options);
        }
        if(myArray[4].trim() != "")
        {
            publish(myArray[0].trim() + "/weight", myArray[4].trim(), options);
        }
        if(myArray[5].trim() != "")
        {
            publish(myArray[0].trim() + "/temp2", myArray[5].trim(), options);
        }
        if(myArray[6].trim() != "")
        {
            publish(myArray[0].trim() + "/hum1", myArray[6].trim(), options);
        }
    }
    else
    {
        console.log("Message is incorrect");
    }


   /* for(i = 0;i<myArray.length ;i++)
    {
        console.log("PArt " + i + "content " + myArray[i]);
    }*/
});

client.on("connect",function()
{	
    console.log("connected  "+ client.connected);
})

//handle errors
client.on("error",function(error)
{
    console.log("Can't connect" + error);
    process.exit(1)
});

//publish
function publish(topic,msg,options)
{
    console.log("publishing",msg);

    if (client.connected == true)
    {	
        client.publish(topic,msg,options);
    }
}

var options=
{
    retain:true,
    qos:1
};

