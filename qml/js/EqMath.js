.pragma library
// ---------------------------------------------------------------------------
//  EQ magnitude-response math for the on-screen EQ curve.
//  Standard biquad/analog-prototype magnitude approximations for bell,
//  shelf and pass filters. Produces a gain (dB) at a given frequency so the
//  EQ graph can be drawn and the combined curve summed across bands.
// ---------------------------------------------------------------------------

function logFreqAt(x, width, fMin, fMax) {
    // map pixel x -> frequency on a log scale
    var t = x / width;
    return fMin * Math.pow(fMax / fMin, t);
}

function xForFreq(f, width, fMin, fMax) {
    return width * Math.log(f / fMin) / Math.log(fMax / fMin);
}

// Peaking / bell band gain (dB) at frequency f.
function bellGainDb(f, fc, gainDb, q) {
    if (q <= 0) q = 0.1;
    var w = f / fc;
    // ratio of deviation from centre, normalised by bandwidth (~1/Q)
    var bw = 1.0 / q;
    var x = (w - 1.0 / w) / bw;
    var shape = 1.0 / (1.0 + x * x);   // 0..1, 1 at centre
    return gainDb * shape;
}

// Low/High shelf gain (dB) at frequency f. type: "low" or "high".
function shelfGainDb(f, fc, gainDb, type) {
    var r = f / fc;
    var s;
    if (type === "low")  s = 1.0 / (1.0 + r * r);      // full gain below fc
    else                 s = (r * r) / (1.0 + r * r);  // full gain above fc
    return gainDb * s;
}

// High-pass / low-pass attenuation (dB) at frequency f for a given slope
// (dB/oct, e.g. 12 or 24). type: "hp" or "lp".
function passGainDb(f, fc, slopeDbOct, type) {
    var oct;
    if (type === "hp") oct = Math.log(f / fc) / Math.log(2);   // below fc -> negative
    else               oct = Math.log(fc / f) / Math.log(2);
    if (oct >= 0) return 0.0;            // passband
    return slopeDbOct * oct;             // stopband roll-off (negative)
}

// Sum a set of band descriptors into one response array of dB values.
// bands: [{enabled, kind:'bell'|'lowshelf'|'highshelf'|'hp'|'lp', fc, gain, q, slope}]
function curve(bands, width, fMin, fMax) {
    var out = new Array(width);
    for (var x = 0; x < width; ++x) {
        var f = logFreqAt(x, width, fMin, fMax);
        var sum = 0;
        for (var i = 0; i < bands.length; ++i) {
            var b = bands[i];
            if (!b.enabled) continue;
            switch (b.kind) {
            case 'bell':      sum += bellGainDb(f, b.fc, b.gain, b.q); break;
            case 'lowshelf':  sum += shelfGainDb(f, b.fc, b.gain, 'low'); break;
            case 'highshelf': sum += shelfGainDb(f, b.fc, b.gain, 'high'); break;
            case 'hp':        sum += passGainDb(f, b.fc, b.slope || 24, 'hp'); break;
            case 'lp':        sum += passGainDb(f, b.fc, b.slope || 24, 'lp'); break;
            }
        }
        out[x] = sum;
    }
    return out;
}
