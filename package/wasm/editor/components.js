webix.protoUI({
    name: "graphedit",
    defaults: {},
    $init: function(cfg) {
        this._initcomplete = false;
        this._canvasId = webix.uid().toString();
        this._valueId = webix.uid().toString();
        this._spreadId = webix.uid().toString();
        this._absmin = cfg.min;
        this._absmax = cfg.max;
        this._selected = null;
        this._isdragging = false;

        let tmpId = webix.uid();
        this._points = {};
        this._points[tmpId] = { id: tmpId, x: -0.01, value: cfg.value, lower: cfg.value, upper: cfg.value, spread: 0.0 };
        console.log("1:", this._canvasId, this._valueId, this._spreadId, tmpId);

        this.vminmax();

        cfg.rows = [
            {
                cols: [
                    { view: "label", label: cfg.title, autowidth: true, height: 24 }, {},
                    {
                        id: this._valueId, view: "text", type: "number", format: "111.00", value: 0.0, height: 24, width: 60,
                        attributes: { step: 0.01, min: cfg.min, max: cfg.max },
                        on: {
                            onEnter: (e) => {
                                const element = $$(this._valueId);
                                const v = Math.max(cfg.min, Math.min(cfg.max, parseFloat(element.getText())));
                                element.setValue(v.toString());
                                element.blur(); 
                            },
                            onChange: (nv, old) => {
                                this.pointValueChanged(nv, old);
                            }
                        }
                    },
                    { view: "label", label: " ± ", autowidth: true, height: 24 },
                    {
                        id: this._spreadId, view: "text", type: "number", format: "111.00", value: 0.0, height: 24, width: 60,
                        attributes: { step: 0.01, min: 0.0, max: cfg.spread },
                        on: {
                            onEnter: (e) => {
                                const element = $$(this._spreadId);
                                const v = Math.max(0.0, Math.min(cfg.spread, parseFloat(element.getText())));
                                element.setValue(v.toString());
                                element.blur(); 
                            },
                            onChange: (nv, old) => {
                                this.pointSpreadChanged(nv, old);
                            }
                        }
                    },
                    { 
                        view: "button", value: "\u29BB", width: 24, height: 24, tooltip: { template: "Remove point", delay: 800 },
                        click: () => { this.pointRemove(); } 
                    },
                ]
            },
            {
                padding: 2.5,
                rows: [{
                    view: "template",
                    css: {"margin-top" : "0px !important"},
                    borderless: true,
                    template: '<canvas id="' + this._canvasId + '" style="width:100%;height:100%;display:block;background-color:#d0d0d0"></canvas>',
                }]
            }
        ];
    },
    $setSize: function(x, y) {
        webix.ui.layout.prototype.$setSize.call(this, x, y);

        if (this._canvas) {
            this._canvas.removeEventListener("pointerdown", this._pointerdownHandler);
            this._canvas.removeEventListener("pointermove", this._pointermoveHandler);
            this._canvas.removeEventListener("pointerup", this._pointerupHandler);
            this._canvas.removeEventListener("dblclick", this._dblclickHandler);            
        }

        this._pointerdownHandler = (e) => {
            var dist = 200;
            this._selected = null;
            for (const [k, v] of Object.entries(this._points)) {
                const coords = this.coords(v.x, v.value);
                const dx = 2.0 * e.offsetX - coords.x;
                const dy = 2.0 * e.offsetY - coords.y;
                const ds = dx * dx + dy * dy;
                if (ds < dist) {
                    dist = ds;
                    this._selected = k;
                    this._isdragging = true;
                    this._canvas.setPointerCapture(e.pointerId);
                }
            }
            this.draw();
            e.preventDefault();
        };
        this._pointermoveHandler = (e) => {
            if (this._selected && this._isdragging) {
                const [fx, fy, fw, fh] = this.field();
                const x = Math.max(0.0, Math.min((2.0 * e.offsetX - fx) / fw, 1.0));
                const y = Math.max(this._absmin, Math.min((1.0 - (2.0 * e.offsetY - fy) / fh) * (this._vmax - this._vmin) + this._vmin, this._absmax));
                let point = this._points[this._selected];
                if (point.x >= 0.0) {
                    point.x = x;
                }
                point.value = y;
                point.lower = Math.max(y - point.spread, this._absmin);
                point.upper = Math.min(y + point.spread, this._absmax);
                this.draw();
            }
        };
        this._pointerupHandler = (e) => {
            this._isdragging = false;
        };
        this._dblclickHandler = (e) => {
            const [fx, fy, fw, fh] = this.field();
            const x = (2.0 * e.offsetX - fx) / fw;
            const y = (1.0 - (2.0 * e.offsetY - fy) / fh) * (this._vmax - this._vmin) + this._vmin;

            if (x >= 0.0 && x <= 1.0 && y >= this._vmin && y <= this._vmax) {
                tmpId = webix.uid();
                this._points[tmpId] = { id: tmpId, x: x, value: y, lower: y, upper: y, spread: 0.0 };
                this._selected = tmpId;
                this.draw();
            }
        };

        this._canvas = document.getElementById(this._canvasId);
        this._canvas.width  = 2.0 * this._canvas.clientWidth;
        this._canvas.height = 2.0 * this._canvas.clientHeight;
        this._ctx = this._canvas.getContext("2d");

        this._canvas.addEventListener("pointerdown", this._pointerdownHandler);
        this._canvas.addEventListener("pointermove", this._pointermoveHandler);
        this._canvas.addEventListener("pointerup", this._pointerupHandler);
        this._canvas.addEventListener("dblclick", this._dblclickHandler);
        this.draw();
    },
    vminmax: function() {
        let vmin = 0.0, vmax = 0.0;
        for (const [k, v] of Object.entries(this._points)) {
            vmax = Math.max(vmax, v.upper);
            vmin = Math.min(vmin, v.lower);
        }
        this._vmin = vmin;
        this._vmax = vmax;
    },
    field: function() {
        const w = this._canvas.width, h = this._canvas.height;
        return [0.08 * w, 0.1 * h, 0.88 * w, 0.8 * h]; // xstart, ystart, width, height
    },
    coords: function(x, y) {
        const [fx, fy, fw, fh] = this.field();
        return {
            x: x * fw + fx, y: (1.0 - (y - this._vmin) / (this._vmax - this._vmin)) * fh + fy
        };
    },
    draw: function() {
        const ctx = this._ctx;
        const w = this._canvas.width, h = this._canvas.height;
        const [fx, fy, fw, fh] = this.field();
        this.vminmax();
        let points = Object.values(this._points).sort((a,b) => (a.x > b.x) ? 1 : ((b.x > a.x) ? -1 : 0));

        $$(this._valueId).blockEvent();
        $$(this._spreadId).blockEvent();
        $$(this._valueId).setValue(this._selected ? this._points[this._selected].value : 0.0);
        $$(this._spreadId).setValue(this._selected ? this._points[this._selected].spread : 0.0);
        $$(this._valueId).unblockEvent();
        $$(this._spreadId).unblockEvent();
        
        let zerooff = 1.0 - (0.0 - this._vmin) / (this._vmax - this._vmin);

        ctx.clearRect(0, 0, w, h);
        ctx.lineWidth = 1;
        ctx.font = "11px arial";
        ctx.beginPath();
        ctx.strokeStyle = "#888888";
        ctx.moveTo(fx, fy);
        ctx.lineTo(fx, fy + fh);
        ctx.stroke();
        ctx.beginPath();
        ctx.strokeStyle = "#888888";
        ctx.moveTo(fx, fy + fh * zerooff);
        ctx.lineTo(fx + fw, fy + fh * zerooff);
        ctx.stroke();
        ctx.textAlign = "right";
        ctx.fillStyle = "#000000";
        ctx.fillText(Number(this._vmax.toFixed(2)).toString(), fx - 0.015 * w, fy + 4);
        ctx.fillText(Number(this._vmin.toFixed(2)).toString(), fx - 0.015 * w, fy + fh + 4);

        var value = this.coords(0.0, points[0].value);
        var lower = this.coords(0.0, points[0].lower);
        var upper = this.coords(0.0, points[0].upper);
        let lastp = points[points.length - 1];
        points.push({ x: 1.0, value: lastp.value, lower: lastp.lower, upper: lastp.upper});
        
        for (let i = 0; i < points.length - 1; i++) {
            const arcposx = value.x, arcposy = value.y;
            ctx.beginPath();
            ctx.strokeStyle = "#888888";
            ctx.moveTo(lower.x, lower.y);
            lower = this.coords(points[i + 1].x, points[i + 1].lower);
            ctx.lineTo(lower.x, lower.y);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(upper.x, upper.y);
            upper = this.coords(points[i + 1].x, points[i + 1].upper);
            ctx.lineTo(upper.x, upper.y);
            ctx.stroke();
            ctx.beginPath();
            ctx.strokeStyle = "#000000";
            ctx.moveTo(value.x, value.y);
            value = this.coords(points[i + 1].x, points[i + 1].value);
            ctx.lineTo(value.x, value.y);
            ctx.stroke();
            ctx.beginPath();
            ctx.strokeStyle = "#000000";
            ctx.arc(arcposx, arcposy, 3, 0, 2 * Math.PI);
            ctx.fillStyle = (points[i].id == this._selected ? "#ff3333" : "#d0d0d0");
            ctx.fill();
            ctx.stroke();
        }
    },
    pointRemove: function() {
        if (this._selected && this._points[this._selected].x >= 0.0) {
            delete this._points[this._selected];
        }
        this._selected = null;
        this.draw();
    },
    pointValueChanged: function(nv, old) {
        if (this._selected) {
            let point = this._points[this._selected];
            point.value = nv;
            point.lower = Math.max(nv - point.spread, this._absmin);
            point.upper = Math.min(nv + point.spread, this._absmax);
            this.draw();
        }
        else {
            $$(this._valueId).blockEvent();
            $$(this._valueId).setValue(0.0);
            $$(this._valueId).unblockEvent();
        }
    },
    pointSpreadChanged: function(nv, old) {
        if (this._selected) {
            let point = this._points[this._selected];
            point.spread = nv;
            point.lower = Math.max(point.value - point.spread, this._absmin);
            point.upper = Math.min(point.value + point.spread, this._absmax);
            this.draw();
        }
        else {
            $$(this._spreadId).blockEvent();
            $$(this._spreadId).setValue(0.0);
            $$(this._spreadId).unblockEvent();
        }
    },
    setValue: function(v) {

    },
    getValue: function() {

    }
}, webix.ui.layout);

let elements = {
    label: function(value, align = "left") {
        return { 
            view: "label", label: value, height: 24, autowidth: true, align: align
        };
    },
    button: function(value, tooltip, action = null) {
        const bid = webix.uid().toString();
        return { 
            id: bid, view: "button",  value: value, width: 24, height: 24, tooltip: { template: tooltip, delay: 800 },
            click: () => { if (action) action($$(bid)); }
        };
    },
    toggle: function(on, off, value, tooltip, action = null) {
        const id = webix.uid().toString();
        return {
            id: id, view: "toggle", offLabel: off, onLabel: on, width: 24, height: 24, value: value,
            tooltip: { template: tooltip, delay: 800 },
            on: {
                onChange: function(nv) {
                    if (action) action($$(id), nv === "true");
                }
            }
        }
    },
    text: function(id, value, width, readonly, action = null) {
        return { 
            id: id, view: "text", type: "text", value: value, width: width, height: 24, readonly: readonly, 
            on: {
                onEnter: function() {
                    this.blur(); 
                },
                onChange: function(nv, old) {
                    if (action) action($$(id), nv, old);
                }
            }
        };
    },
    number: function(id, tooltip, width, format, value, step, min, max, action = null) {
        return { 
            id: id, view: "text", type:"number", format: format, value: value, height: 24, width: width, 
            tooltip: { template: tooltip, delay: 800 },
            on: {
                onEnter: (e) => {
                    let element = $$(id);
                    const currentValue = element.getText(); 
                    const v = Math.max(min, Math.min(max, currentValue));
                    element.setValue(v.toString());
                    element.blur();
                },
                onChange: function(nv, old) {
                    if (action) action($$(id), nv, old);
                }
            },
            attributes:{ step: step, min: min, max: max }
        };
    },
    combo: function(id, value, list, width, action = null) {
        return {
            id: id, view: "combo", container: "editor", value: value, width: width, height: 24, 
            options:{
                view: "suggest",
                body: {
                    view: "list",
                    type: {
                        height: 22
                    },
                    data: list,
                }
            },
            on: {
                onChange: (nv, old) => { if (action) action($$(id), nv, old); }
            }
        };
    },
    checkbox: function(id, value, action = null) {
        return {
            id: id, view: "checkbox", value: value, height: 24,
            on: {
                onChange: (nv, old) => { if (action) action($$(id), nv, old); }
            }
        }
    },
    graphedit: function(id, title, value, min, max, maxspread, action = null) {
        return {
            view: "graphedit", id: id, height: 100, borderless: true, margin: 3, css: {"background-color" : "transparent"},
            title: title, value: value, min: min, max: max, spread: maxspread
        }
    }
}