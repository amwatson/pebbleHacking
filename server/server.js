var querystring = require('querystring');
var http = require('http');

var max_entries = 100;
var time_list = [max_entries];
var num_entries = 0;

var time_display = 0; 

function time_in_secs() {
    var d = new Date();
    return (d.getHours() * 60 * 60) + (d.getMinutes() * 60) + d.getSeconds();
}

function processPost(request, response, callback) {
    var queryData = "";

    if(typeof callback !== 'function') return null;

    if(request.method == 'POST') {
        request.on('data', function(data) {
                console.log("POST data " + data);
                
                // put this in post section
                // getTime() is in milliseconds
                time_list[num_entries%max_entries] = [time_in_secs() + Math.random()%5, time_in_secs()];
                num_entries++;

                queryData += data;
                if(queryData.length > 1e6) {
                queryData = "";
                response.writeHead(413, {'Content-Type': 'text/plain'}).end();
                request.connection.destroy();
                }
                });

        request.on('end', function() {
                request.post = querystring.parse(queryData);
                callback();
                });

    } else {
        response.writeHead(405, {'Content-Type': 'text/plain'});
        response.end();
    }
}


http.createServer(function(request, response) {
        time_sec = time_in_secs(); 

        var i;
        var time_sum = 0;
        for (i = 0; i < num_entries; i++) {
        update_time = time_list[i][0] + ((time_in_secs() - time_list[i][1]));
        time_sum+=update_time;         
        }        
        if (num_entries == 0) {
        time_display = Math.round(time_sec);            
        } else {
        time_display = Math.round(time_sum/num_entries);
        }
        if(request.method == 'POST') {
            processPost(request, response, function() {
            // Use request.post here

            response.writeHead(200, "OK", {'Content-Type': 'text/plain'});
            response.end();
            });
        } else {
            response.writeHead(200, "OK", {'Content-Type': 'text/plain'});
            var time_json = "\{\"main\"\:\{\"ticks\"\:".concat(time_display.toString()).concat("\}\}");
            response.end(time_json); 
        }

}).listen(1337, '128.237.172.249');
console.log("listening on 128.237.172.249:1337");
