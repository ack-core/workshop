class EventType {
    static INIT  = 1
    static READY = 2
    static BTASK = 3
    static FTASK = 4
}

var instance = {};
var memory = {};

onmessage = (msg) => {
    if (msg.data.type == EventType.INIT) {
        memory = msg.data.memory;
        const imports = {
            env: {
                memory: memory,
                abort: function() {
                    throw "aborted";
                },
                js_waiting: () => console.log("[PLATFORM] waiting for memory lock"),
                js_log: function(str, len) {
                    const u16str = new Uint16Array(memory.buffer, str, len);
                    console.log(String.fromCharCode(...u16str));
                },
                js_task: function(task) {
                    throw "can't start task in background";
                },
                js_fetch: function(block, pathLen) {
                    throw "can't read files in background";
                },
                js_canvas_width: function() {
                    throw "can't access canvas from background";
                },
                js_canvas_height: function() {
                    throw "can't access canvas from background";
                },
                webgl_createProgram: function(vsrc, vlen, fsrc, flen) {
                    throw "can't access webgl from background";
                },
                webgl_createData: function(ptr, len) {
                    throw "can't access webgl from background";
                },
                webgl_applyState: function(shaderID) {
                    throw "can't access webgl from background";
                },
                webgl_applyConstants: function(index, ptr, len) {
                    throw "can't access webgl from background";
                },
                webgl_bindBuffer: function(bufferID) {
                    throw "can't access webgl from background";
                },
                webgl_vertexAttribute: function(index, size, type, nrm, stride, offset, divisor) {
                    throw "can't access webgl from background";
                },
                webgl_draw: function(vertexCount, topology) {
                    throw "can't access webgl from background";
                },
                webgl_drawInstanced: function(vertexCount, instanceCount, topology) {
                    throw "can't access webgl from background";
                }
            }
        };

        WebAssembly.instantiate(msg.data.module, imports).then(m => {
            instance = m;
            postMessage({type: EventType.READY});
        });
    }
    if (msg.data.type == EventType.BTASK) {
        const task = msg.data.task;
        instance.exports.task_execute(task);
        postMessage({type: EventType.FTASK, task});
    }
};
