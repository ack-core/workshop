class GraphEdit {
    constructor(container, title, value, min, max, maxspread, onChange) {
        container.innerHTML = `
            <div class="editor_inspect_row">
                <label>${title}</label>
                <input type="number" style="width: 60px" step="0.01" min="${min}" max="${max}">
                <label style="flex: none">&plusmn;</label>
                <input type="number" style="width: 60px" step="0.01" min="0" max="${maxspread}">
                <button class="editor_top_button" title="Remove point">&#x29BB;</button>
            </div>
            <canvas class="editor_inspect_graph"></canvas>`;

        [this._valueInput, this._spreadInput] = container.querySelectorAll("input");
        this._canvas = container.querySelector("canvas");
        this._ctx = this._canvas.getContext("2d");
        this._absmin = min;
        this._absmax = max;
        this._maxspread = maxspread;
        this._onChange = onChange;
        this._selected = null;
        this._isdragging = false;
        let firstPoint = { x: -0.0001 };
        this._setPoint(firstPoint, value, 0.0);
        this._points = [firstPoint];

        this._valueInput.onchange = () => this._inputChanged();
        this._spreadInput.onchange = () => this._inputChanged();
        container.querySelector("button").onclick = () => this.pointRemove();

        this._canvas.onpointerdown = (e) => {
            let dist = 200;
            this._selected = null;
            for (const p of this._points) {
                const coords = this._coords(p.x, p.value);
                const dx = 2.0 * e.offsetX - coords.x;
                const dy = 2.0 * e.offsetY - coords.y;
                const ds = dx * dx + dy * dy;
                if (ds < dist) {
                    dist = ds;
                    this._selected = p;
                }
            }
            if (this._selected) {
                this._isdragging = true;
                this._canvas.setPointerCapture(e.pointerId);
            }
            this.draw();
            e.preventDefault();
        };
        this._canvas.onpointermove = (e) => {
            if (this._selected && this._isdragging) {
                const [fx, fy, fw, fh] = this._field();
                if (this._selected.x >= 0.0) {
                    this._selected.x = Math.max(0.0, Math.min((2.0 * e.offsetX - fx) / fw, 1.0));
                }
                this._setPoint(this._selected, (1.0 - (2.0 * e.offsetY - fy) / fh) * (this._vmax - this._vmin) + this._vmin, this._selected.spread);
                this.draw();
            }
        };
        this._canvas.onpointerup = (e) => {
            if (this._selected && this._isdragging) {
                this._onChange(this.getValue());
            }
            this._isdragging = false;
        };
        this._canvas.ondblclick = (e) => {
            const [fx, fy, fw, fh] = this._field();
            const x = (2.0 * e.offsetX - fx) / fw;
            const y = (1.0 - (2.0 * e.offsetY - fy) / fh) * (this._vmax - this._vmin) + this._vmin;

            if (x >= 0.0 && x <= 1.0 && y >= this._vmin && y <= this._vmax) {
                let point = { x: x };
                this._setPoint(point, y, 0.0);
                this._points.push(point);
                this._selected = point;
                this.draw();
                this._onChange(this.getValue());
            }
        };
        this.draw();
    }
    _setPoint(p, value, spread) {
        p.value = Math.max(this._absmin, Math.min(this._absmax, value));
        p.spread = Math.max(0.0, Math.min(this._maxspread, spread));
        p.lower = Math.max(p.value - p.spread, this._absmin);
        p.upper = Math.min(p.value + p.spread, this._absmax);
    }
    _inputChanged() {
        if (this._selected) {
            this._setPoint(this._selected, parseFloat(this._valueInput.value) || 0.0, parseFloat(this._spreadInput.value) || 0.0);
            this._onChange(this.getValue());
        }
        this.draw();
    }
    _field() {
        const w = this._canvas.width, h = this._canvas.height;
        return [0.08 * w, 0.1 * h, 0.88 * w, 0.8 * h]; // xstart, ystart, width, height
    }
    _coords(x, y) {
        const [fx, fy, fw, fh] = this._field();
        return {
            x: x * fw + fx, y: (1.0 - (y - this._vmin) / (this._vmax - this._vmin)) * fh + fy
        };
    }
    draw() {
        if (this._selected) {
            this._valueInput.value = Number(this._selected.value.toFixed(2));
            this._spreadInput.value = Number(this._selected.spread.toFixed(2));
        }
        else {
            this._valueInput.value = 0.0;
            this._spreadInput.value = 0.0;
        }

        let vmin = 0.0, vmax = 0.0;
        for (const p of this._points) {
            vmax = Math.max(vmax, p.upper);
            vmin = Math.min(vmin, p.lower);
        }
        if (vmax - vmin < 0.0001) {
            vmax = vmin + 0.0001;
        }
        this._vmin = vmin;
        this._vmax = vmax;

        const canvas = this._canvas;
        if (canvas.clientWidth == 0) {
            return;
        }
        canvas.width = 2.0 * canvas.clientWidth;
        canvas.height = 2.0 * canvas.clientHeight;

        const ctx = this._ctx;
        const w = canvas.width, h = canvas.height;
        const [fx, fy, fw, fh] = this._field();
        const points = this.getValue();
        const zerooff = 1.0 - (0.0 - this._vmin) / (this._vmax - this._vmin);

        ctx.lineWidth = 1;
        ctx.font = "10px arial";
        ctx.strokeStyle = "#888888";
        ctx.beginPath();
        ctx.moveTo(fx, fy);
        ctx.lineTo(fx, fy + fh);
        ctx.moveTo(fx, fy + fh * zerooff);
        ctx.lineTo(fx + fw, fy + fh * zerooff);
        ctx.stroke();
        ctx.textAlign = "right";
        ctx.fillStyle = "#000000";
        ctx.fillText(Number(this._vmax.toFixed(2)).toString(), fx - 0.01 * w, fy + 4);
        ctx.fillText(Number(this._vmin.toFixed(2)).toString(), fx - 0.01 * w, fy + fh + 4);

        let value = this._coords(0.0, points[0].value);
        let lower = this._coords(0.0, points[0].lower);
        let upper = this._coords(0.0, points[0].upper);
        const lastp = points[points.length - 1];
        points.push({ x: 1.0, value: lastp.value, lower: lastp.lower, upper: lastp.upper });

        for (let i = 0; i < points.length - 1; i++) {
            const arcposx = value.x, arcposy = value.y;
            ctx.beginPath();
            ctx.strokeStyle = "#888888";
            ctx.moveTo(lower.x, lower.y);
            lower = this._coords(points[i + 1].x, points[i + 1].lower);
            ctx.lineTo(lower.x, lower.y);
            ctx.moveTo(upper.x, upper.y);
            upper = this._coords(points[i + 1].x, points[i + 1].upper);
            ctx.lineTo(upper.x, upper.y);
            ctx.stroke();
            ctx.beginPath();
            ctx.strokeStyle = "#000000";
            ctx.moveTo(value.x, value.y);
            value = this._coords(points[i + 1].x, points[i + 1].value);
            ctx.lineTo(value.x, value.y);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(arcposx, arcposy, 3, 0, 2 * Math.PI);
            ctx.fillStyle = (points[i] == this._selected ? "#ff3333" : "#d0d0d0");
            ctx.fill();
            ctx.stroke();
        }
    }
    pointRemove() {
        if (this._selected && this._selected.x >= 0.0) {
            this._points = this._points.filter(p => p != this._selected);
        }
        this._selected = null;
        this.draw();
        this._onChange(this.getValue());
    }
    setValue(v) {
        this._selected = null;
        if (v.length) {
            const sorted = v.sort((a, b) => a.x - b.x);
            this._points = [];
            for (const p of sorted) {
                const isFirst = this._points.length == 0;
                let point = { x: isFirst ? -0.0001 : p.x };
                this._setPoint(point, p.value, p.spread);
                this._points.push(point);
            }
        }
        this.draw();
    }
    getValue() {
        return [...this._points].sort((a, b) => a.x - b.x);
    }
}
