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
                acosf: Math.acos,
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
                js_editorMsg: (msg, msglen, data, datalen) => { throw "can't message to editor from background" },
                js_canvas_width: () => { throw "can't access canvas from background" },
                js_canvas_height: () => { throw "can't access canvas from background" },
                webgl_createProgram: (vsrc, vlen, fsrc, flen) => { throw "can't access webgl from background" },
                webgl_createData: (layout, layoutLen, ptr, dataLen, stride, idx, icount) => { throw "can't access webgl from background" },
                webgl_createTexture: (format, internal, type, w, h, mipAddresses, mipCount) => { throw "can't access webgl from background" },
                webgl_createTarget: (format, internal, type, w, h, count, enableDepth, textures) => { throw "can't access webgl from background" },
                webgl_deleteProgram: (id) => { throw "can't access webgl from background" },
                webgl_deleteData: (id) => { throw "can't access webgl from background" },
                webgl_deleteTexture: (id) => { throw "can't access webgl from background" },
                webgl_deleteTarget: (id) => { throw "can't access webgl from background" },
                webgl_viewPort: (w, h) => { throw "can't access webgl from background" },
                webgl_beginTarget: (frameBufferID, depth, mask, r, g, b, a, d) => { throw "can't access webgl from background" },
                webgl_endTarget: (frameBufferID, depth) => { throw "can't access webgl from background" },
                webgl_applyShader: (shaderID, ztype, btype) => { throw "can't access webgl from background" },
                webgl_applyConstants: (index, ptr, len) => { throw "can't access webgl from background" },
                webgl_applyTexture: (index, textureID, samplingType) => { throw "can't access webgl from background" },
                webgl_drawDefault: (buffer, vertexCount, instanceCount, topology) => { throw "can't access webgl from background" },
                webgl_drawIndexed: (buffer, indexes, indexCount, topology) => { throw "can't access webgl from background" },
                webgl_drawWithRepeat: (buffer, attrCount, instanceCount, vertexCount, totalInstCount, topology) => { throw "can't access webgl from background" }
            }
        };

        WebAssembly.instantiate(msg.data.module, imports).then(m => {
            instance = m;
            postMessage({type: EventType.READY});
        });
    }
    if (msg.data.type == EventType.BTASK) {
        instance.exports.taskExecute(msg.data.task);
        postMessage({type: EventType.FTASK, task: msg.data.task});
    }
};
