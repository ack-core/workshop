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
                tanf: Math.tan,
                sinf: Math.sin,
                cosf: Math.cos,
                abort: function() {
                    throw "aborted";
                },
                js_waiting: () => console.log("[PLATFORM] waiting for memory lock"),
                js_log: function(str, len) {
                    const u16str = new Uint16Array(memory.buffer, str, len);
                    console.log(String.fromCharCode(...u16str));
                },
                js_task: (task) => { throw "can't start task in background" },
                js_fetch: (block, pathLen) => { throw "can't read files in background" },
                js_canvas_width: () => { throw "can't access canvas from background" },
                js_canvas_height: () => { throw "can't access canvas from background" },
                webgl_createProgram: (vsrc, vlen, fsrc, flen) => { throw "can't access webgl from background" },
                webgl_createData: (ptr, len) => { throw "can't access webgl from background" },
                webgl_createTexture: (format, internal, type, w, h, mipAddresses, mipCount) => { throw "can't access webgl from background" },
                webgl_createTarget: (format, internal, type, w, h, count, enableDepth, textures) => { throw "can't access webgl from background" },
                webgl_viewPort: (w, h) => { throw "can't access webgl from background" },
                webgl_applyState: (shaderID, mask, r, g, b, a, d, ztype, btype) => { throw "can't access webgl from background" },
                webgl_applyConstants: (index, ptr, len) => { throw "can't access webgl from background" },
                webgl_applyTexture: (index, textureID, samplingType) => { throw "can't access webgl from background" },
                webgl_bindBuffer: (bufferID) => { throw "can't access webgl from background" },
                webgl_enableAttributes: (count) => { throw "can't access webgl from background" },
                webgl_inputFAttributePtr: (index, size, type, nrm, stride, offset) => { throw "can't access webgl from background" },
                webgl_inputIAttributePtr: (index, size, type, nrm, stride, offset) => { throw "can't access webgl from background" },
                webgl_inputAttributeDiv: (index, divisor) => { throw "can't access webgl from background" },                
                webgl_drawSingle: (vertexCount, topology) => { throw "can't access webgl from background" },
                webgl_drawInstanced: (vertexCount, instanceCount, topology) => { throw "can't access webgl from background" },
                webgl_drawInstanceIndex: (shaderID, repeatCount, vertexCount, instanceIndex, topology) => { throw "can't access webgl from background" }
            }
        };

        WebAssembly.instantiate(msg.data.module, imports).then(m => {
            instance = m;
            postMessage({type: EventType.READY});
        });
    }
    if (msg.data.type == EventType.BTASK) {
        const task = msg.data.task;
        instance.exports.taskExecute(task);
        postMessage({type: EventType.FTASK, task});
    }
};
