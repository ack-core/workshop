const output = document.getElementById("output");
function print(...a) {
    output.innerHTML += a.join(" ") + "<br>";
    console.log(...a);
}


if ("serviceWorker" in navigator) {
    navigator.serviceWorker.register("serviceworker.js").then(
        (registration) => {
            print("Custom header service worker registered: ", registration.scope);
        },
        (error) => {
            throw "Custom header service worker failed to register: " + error;
        }
    );
    navigator.serviceWorker.ready.then(
        (registration) => {
            if (registration.active && !navigator.serviceWorker.controller) {
                window.location.reload();
            }
        }
    );
} 
else {
    throw "Service workers are not supported";
}


if (!crossOriginIsolated) {
    print("[PLATFORM] Page is not cross-origin isolated");
}

const WASM_BINARY = document.currentScript.getAttribute('binary');
const BUFFERS_REUSE_MAX = 256;
const FRAME_CONST_BINDING_INDEX = 0;
const DRAW_CONST_BINDING_INDEX = 1;
const POINTER_DOWN = 1;
const POINTER_MOVE = 2;
const POINTER_UP = 3;
const POINTER_CANCEL = 4;

class EventType {
    static INIT  = 1
    static READY = 2
    static BTASK = 3
    static FTASK = 4
}

var instance = null;
var memory = new WebAssembly.Memory({ initial: 4, maximum: 4096, shared: true });
var worker = new Worker("worker.js");
var canvas = document.getElementById("render_target");
var targetWidth = window.devicePixelRatio * canvas.scrollWidth;
var targetHeight = window.devicePixelRatio * canvas.scrollHeight;
var activePointerIDs = {};
var prevFrameTimeStamp = 0;
var glShaderIDCounter = 0x10000000;
var glBufferIDCounter = 0x20000000;
var glTextureIDCounter = 0x30000000;
var glFrameBufferIDCounter = 0x40000000;
var glshaders = {};
var glbuffers = {};
var gltextures = {};
var glsamplers = {};
var glframebuffers = {};
var glcontext = canvas.getContext("webgl2", { antialias: false, depth: false, stencil: false, premultipliedAlpha: true, preserveDrawingBuffer: true });
var glEnabledAttribCount = 0;
var shaderConstantBuffers = [];
var shaderConstantIndex = 0;
var uniformInstanceCountLocation = null;
var resizeTimeoutHandler = null;

const vertexAttribFunctions = [
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  2, glcontext.HALF_FLOAT, false, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  4, glcontext.HALF_FLOAT, false, stride, offset); return 8; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  1, glcontext.FLOAT, false, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  2, glcontext.FLOAT, false, stride, offset); return 8; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  3, glcontext.FLOAT, false, stride, offset); return 12; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  4, glcontext.FLOAT, false, stride, offset); return 16; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 2, glcontext.SHORT, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 4, glcontext.SHORT, stride, offset); return 8; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 2, glcontext.UNSIGNED_SHORT, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 4, glcontext.UNSIGNED_SHORT, stride, offset); return 8; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  2, glcontext.SHORT, true, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  4, glcontext.SHORT, true, stride, offset); return 8; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  2, glcontext.UNSIGNED_SHORT, true, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  4, glcontext.UNSIGNED_SHORT, true, stride, offset); return 8; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 4, glcontext.UNSIGNED_BYTE, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribPointer(index,  4, glcontext.UNSIGNED_BYTE, true, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 1, glcontext.INT, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 2, glcontext.INT, stride, offset); return 8; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 3, glcontext.INT, stride, offset); return 12; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 4, glcontext.INT, stride, offset); return 16; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 1, glcontext.UNSIGNED_INT, stride, offset); return 4; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 2, glcontext.UNSIGNED_INT, stride, offset); return 8; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 3, glcontext.UNSIGNED_INT, stride, offset); return 12; },
    (index, stride, offset) => { glcontext.vertexAttribIPointer(index, 4, glcontext.UNSIGNED_INT, stride, offset); return 16; },
];
const dtypes = {
    0: function() {},
    1: function() {
        glcontext.disable(glcontext.DEPTH_TEST);
        glcontext.depthMask(true);
    },
    2: function() {
        glcontext.enable(glcontext.DEPTH_TEST);
        glcontext.depthMask(false);
    },
    3: function() {
        glcontext.enable(glcontext.DEPTH_TEST);
        glcontext.depthMask(true);
    },            
};
const btypes = {
    0: function() {},
    1: function() {
        glcontext.disable(glcontext.BLEND);
    },
    2: function() {
        glcontext.enable(glcontext.BLEND);
        glcontext.blendEquation(glcontext.FUNC_ADD);
        glcontext.blendFunc(glcontext.ONE, glcontext.ONE_MINUS_SRC_ALPHA);
    },
    3: function() {
        glcontext.enable(glcontext.BLEND);
        glcontext.blendEquation(glcontext.FUNC_ADD);
        glcontext.blendFuncSeparate(glcontext.SRC_ALPHA, glcontext.ONE, glcontext.ONE, glcontext.ONE);
    },
};

const imports = {
    env: {
        memory: memory,
        tanf: Math.tan,
        sinf: Math.sin,
        cosf: Math.cos,
        js_waiting: () => console.log("[PLATFORM] waiting for memory lock"),
        js_log: function(str, len) {
            const u16str = new Uint16Array(memory.buffer, str, len);
            print(String.fromCharCode(...u16str));
        },
        js_task: function(task) {
            worker.postMessage({type: EventType.BTASK, task});
        },
        js_fetch: function(block, pathLen) {
            const u16path = new Uint16Array(memory.buffer, block, pathLen);
            const path = String.fromCharCode(...u16path);
            
            fetch(path).then(response => {
                if (response.ok) {
                    return response.arrayBuffer();
                }
                throw response.status + " " + response.statusText;
            }).then(buffer => {
                const data = instance.exports.malloc(buffer.byteLength);
                const u8data = new Uint8Array(memory.buffer, data, buffer.byteLength);
                u8data.set(new Uint8Array(buffer));
                instance.exports.fileLoaded(block, pathLen, data, buffer.byteLength);
                print("[WASM] " + path + " loaded successfully");
            })
            .catch(error => {
                print("[WASM] " + path + " loading failed with " + error);
                if (typeof error === "string") {
                    instance.exports.fileLoaded(block, pathLen, 0, 0);
                }
            });
        },
        abort: function() {
            throw "aborted";
        },
        js_dbg: function(s, value) {
            console.log("M--->>> ", s, " | ", value);
        },
        webgl_createProgram: function(vsrc, vlen, fsrc, flen) {
            const vsstr = String.fromCharCode(...new Uint16Array(memory.buffer, vsrc, vlen));
            const fsstr = String.fromCharCode(...new Uint16Array(memory.buffer, fsrc, flen));
            const vshader = glcontext.createShader(glcontext.VERTEX_SHADER);
            const fshader = glcontext.createShader(glcontext.FRAGMENT_SHADER);
            const program = glcontext.createProgram();

            glcontext.shaderSource(vshader, vsstr);
            glcontext.shaderSource(fshader, fsstr);
            glcontext.compileShader(vshader);
            glcontext.compileShader(fshader);

            if (glcontext.getShaderParameter(vshader, glcontext.COMPILE_STATUS)) {
                if (glcontext.getShaderParameter(fshader, glcontext.COMPILE_STATUS)) {
                    glcontext.attachShader(program, vshader);
                    glcontext.attachShader(program, fshader);
                    glcontext.linkProgram(program);
                    glcontext.validateProgram(program);
                    glcontext.uniformBlockBinding(program, glcontext.getUniformBlockIndex(program, "_FrameData"), FRAME_CONST_BINDING_INDEX);

                    const cIndex = glcontext.getUniformBlockIndex(program, "_Constants");
                    if (cIndex != 0xFFFFFFFF) {
                        glcontext.uniformBlockBinding(program, glcontext.getUniformBlockIndex(program, "_Constants"), DRAW_CONST_BINDING_INDEX);
                    }
                    
                    program.uniformSamplersLocation = glcontext.getUniformLocation(program, "_samplers");
                    program.uniformInstanceCountLocation = glcontext.getUniformLocation(program, "_instance_count");

                    if (glcontext.getProgramParameter(program, glcontext.LINK_STATUS)) {
                        glcontext.deleteShader(vshader);
                        glcontext.deleteShader(fshader);
                        glshaders[glShaderIDCounter] = program;
                        return glShaderIDCounter++;
                    }
                    else {
                        print("[WebGL link] " + glcontext.getProgramInfoLog(program));
                        print(vsstr + "-----\n" + fsstr);
                    }
                }
                else {
                    print("[WebGL fs] " + glcontext.getShaderInfoLog(fshader));
                    print(fsstr);
                }
            }
            else {
                print("[WebGL vs] " + glcontext.getShaderInfoLog(vshader));
                print(vsstr);
            }

            glcontext.deleteShader(vshader);
            glcontext.deleteShader(fshader);
            glcontext.deleteProgram(program);
            return 0;
        },
        webgl_createData: function(layout, layoutLen, ptr, dataLen, stride, idx, icount) {
            const u8mem = new Uint8Array(memory.buffer, ptr, dataLen);
            const lmem = new Uint8Array(memory.buffer, layout, layoutLen);
            const vao = glcontext.createVertexArray();
            const vbo = glcontext.createBuffer();
            var ibo = null;

            glcontext.bindVertexArray(vao);
            glcontext.bindBuffer(glcontext.ARRAY_BUFFER, vbo);
            glcontext.bufferData(glcontext.ARRAY_BUFFER, u8mem, glcontext.STATIC_DRAW);

            for (let i = 0, offset = 0; i < layoutLen; i++) {
                glcontext.enableVertexAttribArray(i);
                offset += vertexAttribFunctions[lmem[i]](i, stride, offset);
            }

            if (idx) {
                const imem = new Uint32Array(memory.buffer, idx, icount);
                ibo = glcontext.createBuffer();
                glcontext.bindBuffer(glcontext.ELEMENT_ARRAY_BUFFER, ibo);
                glcontext.bufferData(glcontext.ELEMENT_ARRAY_BUFFER, imem, glcontext.STATIC_DRAW);
            }
            
            glcontext.bindVertexArray(null);
            glcontext.bindBuffer(glcontext.ELEMENT_ARRAY_BUFFER, null);
            glcontext.bindBuffer(glcontext.ARRAY_BUFFER, null);
            glbuffers[glBufferIDCounter] = {
                indexes: ibo,
                buffer: vbo,
                layout: vao
            };

            return glBufferIDCounter++;
        },
        webgl_createTexture(format, internal, type, size, w, h, mipAddresses, mipCount) {
            const mipPtrs = new Uint32Array(memory.buffer, mipAddresses, mipCount);
            const texture = glcontext.createTexture();
            glcontext.activeTexture(glcontext.TEXTURE0);
            glcontext.bindTexture(glcontext.TEXTURE_2D, texture);
            glcontext.texStorage2D(glcontext.TEXTURE_2D, mipCount, internal, w, h);

            for (let i = 0; i < mipCount; i++) {
                const mw = w >> i;
                const mh = h >> i;
                const mipData = new Uint8Array(memory.buffer, mipPtrs[i], mw * mh * size);
                glcontext.texSubImage2D(glcontext.TEXTURE_2D, i, 0, 0, mw, mh, format, type, mipData);
            }

            glcontext.bindTexture(glcontext.TEXTURE_2D, null);
            gltextures[glTextureIDCounter] = texture;
            return glTextureIDCounter++;
        },
        webgl_createTarget(format, internal, type, w, h, count, enableDepth, textures) {
            const texarray = new Uint32Array(memory.buffer, textures, count + 1);
            const frameBuffer = glcontext.createFramebuffer();

            texarray[0] = 0;
            glcontext.bindFramebuffer(glcontext.FRAMEBUFFER, frameBuffer);
            glcontext.activeTexture(glcontext.TEXTURE0);
            var result = {fb: frameBuffer, depth: null, attachments: []};

            if (enableDepth) {
                const texture = glcontext.createTexture();
                glcontext.bindTexture(glcontext.TEXTURE_2D, texture);
                glcontext.texImage2D(glcontext.TEXTURE_2D, 0, glcontext.DEPTH_COMPONENT32F, w, h, 0, glcontext.DEPTH_COMPONENT, glcontext.FLOAT, null);
                glcontext.bindTexture(glcontext.TEXTURE_2D, null);
                glcontext.framebufferTexture2D(glcontext.FRAMEBUFFER, glcontext.DEPTH_ATTACHMENT, glcontext.TEXTURE_2D, texture, 0);
                gltextures[glTextureIDCounter] = texture;
                texarray[0] = glTextureIDCounter++;
                result.depth = texture;
            }
            for (let i = 0; i < count; i++) {
                const texture = glcontext.createTexture();
                glcontext.bindTexture(glcontext.TEXTURE_2D, texture);
                glcontext.texImage2D(glcontext.TEXTURE_2D, 0, internal, w, h, 0, format, type, null);
                glcontext.bindTexture(glcontext.TEXTURE_2D, null);
                glcontext.framebufferTexture2D(glcontext.FRAMEBUFFER, glcontext.COLOR_ATTACHMENT0 + i, glcontext.TEXTURE_2D, texture, 0);
                gltextures[glTextureIDCounter] = texture;
                texarray[i + 1] = glTextureIDCounter++;
                result.attachments[i] = glcontext.COLOR_ATTACHMENT0 + i;
            }
            if (glcontext.checkFramebufferStatus(glcontext.FRAMEBUFFER) != glcontext.FRAMEBUFFER_COMPLETE) {
                print("[WebGL] Framebuffer is not complete");
                return 0;
            }

            glcontext.bindFramebuffer(glcontext.FRAMEBUFFER, null);
            glframebuffers[glFrameBufferIDCounter] = result;
            return glFrameBufferIDCounter++;
        },
        webgl_deleteProgram(id) {
            const program = glshaders[id]
            glcontext.deleteProgram(program);
            glshaders[id] = null;
        },
        webgl_deleteData(id) {
            const data = glbuffers[id]
            glcontext.deleteBuffer(data.vbo);
            glcontext.deleteBuffer(data.ibo);
            glcontext.deleteVertexArray(data.vao);
            glbuffers[id] = null;
        },
        webgl_deleteTexture(id) {
            const texture = gltextures[id]
            glcontext.deleteTexture(texture);
            gltextures[id] = null;
        },
        webgl_deleteTarget(id) {
            const target = glframebuffers[id]
            glcontext.deleteFramebuffer(target.fb);
            glframebuffers[id] = null;
        },
        webgl_viewPort(w, h) {
            glcontext.viewport(0, 0, w, h);
        },
        webgl_beginTarget: function(frameBufferID, depth, mask, r, g, b, a, d) {
            const target = glframebuffers[frameBufferID];
            glcontext.bindFramebuffer(glcontext.FRAMEBUFFER, target.fb);
            
            if (depth) {
                const depthTexture = gltextures[depth]
                glcontext.framebufferTexture2D(glcontext.FRAMEBUFFER, glcontext.DEPTH_ATTACHMENT, glcontext.TEXTURE_2D, depthTexture, 0);
            }

            glcontext.drawBuffers(target.attachments);

            if (mask) {
                glcontext.depthMask(true);
                glcontext.clearColor(r, g, b, 1.0);
                glcontext.clearDepth(d);
                glcontext.clear(mask);
            }
        },
        webgl_endTarget: function(frameBufferID, depth) {
            const target = glframebuffers[frameBufferID];
            if (depth) {
                glcontext.framebufferTexture2D(glcontext.FRAMEBUFFER, glcontext.DEPTH_ATTACHMENT, glcontext.TEXTURE_2D, target.depth, 0);
                glcontext.bindFramebuffer(glcontext.FRAMEBUFFER, null);
            }
        },
        webgl_applyShader: function(shaderID, dtype, btype) {
            const program = glshaders[shaderID];

            glcontext.useProgram(program);
            if (program.uniformSamplersLocation) {
                glcontext.uniform1iv(program.uniformSamplersLocation, [0, 1, 2, 3])
            }
            uniformInstanceCountLocation = program.uniformInstanceCountLocation;

            dtypes[dtype]();
            btypes[btype]();
        },
        webgl_applyConstants: function(index, ptr, len) {
            const u8mem = new Uint8Array(memory.buffer, ptr, len);
            glcontext.bindBuffer(glcontext.UNIFORM_BUFFER, shaderConstantBuffers[shaderConstantIndex]);
            glcontext.bufferData(glcontext.UNIFORM_BUFFER, u8mem, glcontext.DYNAMIC_DRAW);
            glcontext.bindBufferBase(glcontext.UNIFORM_BUFFER, index, shaderConstantBuffers[shaderConstantIndex]);
            shaderConstantIndex = (shaderConstantIndex + 1) % BUFFERS_REUSE_MAX;
        },
        webgl_applyTexture: function(index, textureID, samplingType) {
            const texture = gltextures[textureID];
            glcontext.activeTexture(glcontext.TEXTURE0 + index);
            glcontext.bindTexture(glcontext.TEXTURE_2D, texture);
            glcontext.bindSampler(index, glsamplers[samplingType]);
        },
        webgl_drawDefault: function(buffer, vertexCount, instanceCount, topology) {
            if (uniformInstanceCountLocation) {
                glcontext.uniform1i(uniformInstanceCountLocation, instanceCount);
            }
            glcontext.bindVertexArray(glbuffers[buffer].layout);
            glcontext.drawArraysInstanced(topology, 0, vertexCount, instanceCount);
        },
        webgl_drawIndexed: function(buffer, indexCount, instanceCount, topology) {
            glcontext.bindVertexArray(glbuffers[buffer].layout);
            glcontext.drawElementsInstanced(topology, indexCount, glcontext.UNSIGNED_INT, 0, instanceCount);
        },
        webgl_drawWithRepeat: function(buffer, attrCount, instanceCount, vertexCount, totalInstCount, topology) {
            if (uniformInstanceCountLocation) {
                glcontext.uniform1i(uniformInstanceCountLocation, instanceCount);
            }
            glcontext.bindVertexArray(glbuffers[buffer].layout);
            for (let i = 0; i < attrCount; i++) {
                glcontext.vertexAttribDivisor(i, instanceCount);
            }
            glcontext.drawArraysInstanced(topology, 0, vertexCount, totalInstCount);
        }
    }
};

//navigator.serviceWorker.controller != null && 
if (glcontext != null) {
    WebAssembly.instantiateStreaming(fetch(WASM_BINARY), imports).then(m => {
        instance = m.instance;
        instance.exports.__init();

        if (instance.exports.__wasm_call_ctors) {
            instance.exports.__wasm_call_ctors();
        }

        const module = m.module;
        worker.postMessage({type: EventType.INIT, module, memory}); //
    });
}
else {
    throw "[WebGL] Failed to initialize";
}

function recreateDefaultFBO() {
    const defaultFBO = glcontext.createFramebuffer();
    const defaultDepth = glcontext.createTexture();
    const defaultTarget = glcontext.createTexture();
    glcontext.activeTexture(glcontext.TEXTURE0);
    glcontext.bindTexture(glcontext.TEXTURE_2D, defaultTarget);
    glcontext.texImage2D(glcontext.TEXTURE_2D, 0, glcontext.RGBA8, targetWidth, targetHeight, 0, glcontext.RGBA, glcontext.UNSIGNED_BYTE, null);
    glcontext.bindTexture(glcontext.TEXTURE_2D, defaultDepth);
    glcontext.texImage2D(glcontext.TEXTURE_2D, 0, glcontext.DEPTH_COMPONENT32F, targetWidth, targetHeight, 0, glcontext.DEPTH_COMPONENT, glcontext.FLOAT, null);
    glcontext.bindTexture(glcontext.TEXTURE_2D, null);
    glcontext.bindFramebuffer(glcontext.FRAMEBUFFER, defaultFBO);
    glcontext.framebufferTexture2D(glcontext.FRAMEBUFFER, glcontext.COLOR_ATTACHMENT0, glcontext.TEXTURE_2D, defaultTarget, 0);
    glcontext.framebufferTexture2D(glcontext.FRAMEBUFFER, glcontext.DEPTH_ATTACHMENT, glcontext.TEXTURE_2D, defaultDepth, 0);
    glcontext.bindFramebuffer(glcontext.FRAMEBUFFER, null);
    glframebuffers[0] = {fb: defaultFBO, depth: defaultDepth, attachments: [glcontext.COLOR_ATTACHMENT0]};
}

worker.onmessage = (msg) => {
    if (msg.data.type == EventType.READY) {
        print("[PLATFORM] Background Thread started");

        canvas.width = targetWidth;
        canvas.height = targetHeight;
        
        if (glcontext.getParameter(glcontext.MAX_COLOR_ATTACHMENTS) < 4) {
            print("[WebGL] Max fb attachment count is too low");
            throw new Error;
        }
        if (glcontext.getExtension("EXT_color_buffer_float") == null) {
            print("[WebGL] Float textures aren't supported");
            throw new Error;
        }

        glsamplers[0] = glcontext.createSampler();
        glcontext.samplerParameteri(glsamplers[0], glcontext.TEXTURE_MIN_FILTER, glcontext.NEAREST);
        glcontext.samplerParameteri(glsamplers[0], glcontext.TEXTURE_MAG_FILTER, glcontext.NEAREST);
        glcontext.samplerParameteri(glsamplers[0], glcontext.TEXTURE_WRAP_S, glcontext.REPEAT);
        glcontext.samplerParameteri(glsamplers[0], glcontext.TEXTURE_WRAP_T, glcontext.REPEAT);

        glsamplers[1] = glcontext.createSampler();
        glcontext.samplerParameteri(glsamplers[1], glcontext.TEXTURE_MIN_FILTER, glcontext.LINEAR);
        glcontext.samplerParameteri(glsamplers[1], glcontext.TEXTURE_MAG_FILTER, glcontext.LINEAR);
        glcontext.samplerParameteri(glsamplers[1], glcontext.TEXTURE_WRAP_S, glcontext.REPEAT);
        glcontext.samplerParameteri(glsamplers[1], glcontext.TEXTURE_WRAP_T, glcontext.REPEAT);

        for (let i = 0; i < BUFFERS_REUSE_MAX; i++) {
            shaderConstantBuffers[i] = glcontext.createBuffer();
        }
        
        glbuffers[0] = {
            buffer: glcontext.createBuffer(),
            layout: glcontext.createVertexArray()
        };
        glcontext.bindVertexArray(glbuffers[0].layout);
        glcontext.bindBuffer(glcontext.ARRAY_BUFFER, glbuffers[0].buffer);
        glcontext.enableVertexAttribArray(0);
        glcontext.vertexAttribPointer(0, 1, glcontext.FLOAT, false, 0, 0);
        glcontext.bindVertexArray(null);
        glcontext.bindBuffer(glcontext.ARRAY_BUFFER, null);
        glcontext.depthFunc(glcontext.GREATER);
        
        recreateDefaultFBO();
        instance.exports.initialize();
        instance.exports.resized(targetWidth, targetHeight);

        function getCoord(event) {
            const rect = canvas.getBoundingClientRect();
            const x = (event.clientX - rect.left) * window.devicePixelRatio;
            const y = (event.clientY - rect.top) * window.devicePixelRatio;
            return [x, y];
        }
        function onFrame(timestamp) {
            const interval = 1000.0 / 15.0;
            const dt = timestamp - prevFrameTimeStamp;
            
            if (dt > interval) {
                prevFrameTimeStamp = timestamp;
                glcontext.viewport(0, 0, targetWidth, targetHeight);
                instance.exports.updateFrame(dt);

                glcontext.bindFramebuffer(glcontext.READ_FRAMEBUFFER, glframebuffers[0].fb);
                glcontext.bindFramebuffer(glcontext.DRAW_FRAMEBUFFER, null);
                glcontext.blitFramebuffer(0, targetHeight, targetWidth, 0, 0, 0, targetWidth, targetHeight, glcontext.COLOR_BUFFER_BIT, glcontext.NEAREST);
            }

            //throw "test";

            window.requestAnimationFrame(onFrame);
            uniformInstanceCountLocation = null;
        }
        window.requestAnimationFrame(timestamp => {
            prevFrameTimeStamp = timestamp;
            window.requestAnimationFrame(onFrame);
        });
        canvas.addEventListener("pointerdown", (event) => {
            [x, y] = getCoord(event);
            if (x >= 0 && y >= 0 && x <= targetWidth && y <= targetHeight) {
                activePointerIDs[event.pointerId] = true;
                instance.exports.pointerEvent(POINTER_DOWN, event.pointerId, x, y);
                event.preventDefault();
            }
        });
        canvas.addEventListener("pointermove", (event) => {
            if (activePointerIDs[event.pointerId]) {
                [x, y] = getCoord(event);
                instance.exports.pointerEvent(POINTER_MOVE, event.pointerId, x, y);
                event.preventDefault();
            }
        });
        canvas.addEventListener("pointerup", (event) => {
            if (activePointerIDs[event.pointerId]) {
                delete activePointerIDs[event.pointerId];
                [x, y] = getCoord(event);
                instance.exports.pointerEvent(POINTER_UP, event.pointerId, x, y);
                event.preventDefault();
            }
        });
        canvas.addEventListener("pointercancel", (event) => {
            if (activePointerIDs[event.pointerId]) {
                delete activePointerIDs[event.pointerId];
                [x, y] = getCoord(event);
                instance.exports.pointerEvent(POINTER_CANCEL, event.pointerId, x, y);
                event.preventDefault();
            }
        });
        
        window.addEventListener("resize", (event) => {
            clearTimeout(resizeTimeoutHandler);
            resizeTimeoutHandler = setTimeout(() => {
                const w = window.devicePixelRatio * canvas.scrollWidth;
                const h = window.devicePixelRatio * canvas.scrollHeight;

                if (w > h) {
                    if (w != targetWidth || h != targetHeight) {
                        targetWidth = canvas.width = w;
                        targetHeight = canvas.height = h;
                        recreateDefaultFBO();
                        instance.exports.resized(targetWidth, targetHeight);
                        print("[PLATFORM] Resized to " + targetWidth + " " + targetHeight);
                    }
                }
            }, 300);
        });

        window.addEventListener("contextmenu", (event) => {
            event.preventDefault();
            return false;
        });
        window.addEventListener("touchend", (event) => {
            event.preventDefault();
            return false;
        });
    }
    if (msg.data.type == EventType.FTASK) {
        instance.exports.taskComplete(msg.data.task);
    }
};
