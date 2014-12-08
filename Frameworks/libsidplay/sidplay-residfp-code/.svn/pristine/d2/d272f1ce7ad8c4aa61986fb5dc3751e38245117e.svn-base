using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;

namespace sidwaveforms {
    class Parameters {
        public float bias, pulsestrength, topbit, distance, stmix;

        public int Score(int wave, int[] reference, bool print, int bestscore) {
            var score = 0;
            var wa = new float[12 + 12 + 1];
            for (var i = 0; i <= 12; i ++) {
                wa[12-i] = wa[12+i] = 1.0f / (1.0f + i * i * distance);
            }
            for (var j = 4095; j >= 0; j --) {

                /* S */
                var bitarray = new float[12];
                for (var i = 0; i < 12; i ++)
                    bitarray[i] = (j & (1 << i)) != 0 ? 1f : 0f;

                /* T */
                if ((wave & 3) == 1) {
                    var top = (j & 2048) != 0;
                    for (var i = 11; i > 0; i --) {
                        if (top) {
                            bitarray[i] = 1f - bitarray[i-1];
                        } else {
                            bitarray[i] = bitarray[i-1];
                        }
                    }
                    bitarray[0] = 0;
                }

                /* ST */
                if ((wave & 3) == 3) {
                    bitarray[0] *= stmix;
                    for (var i = 1; i < 12; i ++) {
                        bitarray[i] = bitarray[i-1] * (1f - stmix) + bitarray[i] * stmix;
                    }
                }
                
                bitarray[11] *= topbit;

                SimulateMix(bitarray, wa, wave > 4);

                var simval = GetScore8(bitarray);
                var refval = reference[j];
                score += ScoreResult(simval, refval);

                if (print) {
                    float analogval = 0;
                    for (int i = 0; i < 12; i ++) {
                        float val = (bitarray[i] - bias) * 512 + 0.5f;
                        if (val < 0)
                            val = 0;
                        if (val > 1)
                            val = 1;
                        analogval += val * (1 << i);
                    }
                    analogval /= 16f;
                    Console.WriteLine(string.Format(CultureInfo.InvariantCulture, "{0} {1} {2} {3}", j, refval, simval, analogval));
                }

                if (score > bestscore) {
                    return score;
                }
            }
            return score;
        }

        private void SimulateMix(float[] bitarray, float[] wa, bool HasPulse) {
            var tmp = new float[12];

            for (int sb = 0; sb < 12; sb ++) {
                var n = 0f;
                var avg = 0f;
                for (var cb = 0; cb < 12; cb ++) {
                    var weight = wa[sb - cb + 12];
                    avg += bitarray[cb] * weight;
                    n += weight;
                }
                if (HasPulse) {
                    var weight = wa[sb - 12 + 12];
                    avg += pulsestrength * weight;
                    n += weight;
                }
                tmp[sb] = bitarray[sb] * 0.50f + avg / n * 0.50f;
            }
            for (var i = 0; i < 12; i ++)
                bitarray[i] = tmp[i];
        }

        private int GetScore8(float[] bitarray) {
            var result = 0;
            for (var cb = 0; cb < 8; cb ++) {
                if (bitarray[4+cb] > bias)
                    result |= 1 << cb;
            }
            return result;
        }

        private static int ScoreResult(int a, int b) {
            var v = a ^ b;
            return v;
/*            var c = 0;
            while (v != 0) {
                v &= v - 1;
                c ++;
            }
            return c;
*/
        }
        
        public override string ToString() {
            return string.Format(CultureInfo.InvariantCulture,
@"bestparams.bias = {0}f;
bestparams.pulsestrength = {1}f;
bestparams.topbit = {2}f;
bestparams.distance = {3}f;
bestparams.stmix = {4}f;
new waveformconfig_t({0}f, {1}f, {2}f, {3}f, {4}f)",
                bias, pulsestrength, topbit, distance, stmix
            );
        }
    }

    class Optimizer {
        static readonly Random random = new Random();

        private static float GetRandomValue() {
            var t = 1f - (float) random.NextDouble() * 0.9f;
            if (random.NextDouble() > 0.5) {
                return 1f / t;
            } else {
                return t;
            }
        }

        private static void Optimize(int[] reference, int wave, char chip)
        {
            var bestparams = new Parameters();
            if (chip == 'D') {
                switch (wave) {
                    case 3:
// current score 240
bestparams.bias = 0.9634957f;
bestparams.pulsestrength = 0f;
bestparams.topbit = 0f;
bestparams.distance = 4.165269f;
bestparams.stmix = 0.8020396f;
                    break;
                    case 5:
// current score 600
bestparams.bias = 0.8931507f;
bestparams.pulsestrength = 2.483499f;
bestparams.topbit = 1.0f;
bestparams.distance = 0.03339716f;
bestparams.stmix = 0f;
                    break;
                    case 6:
// current score 613
bestparams.bias = 0.8869214f;
bestparams.pulsestrength = 2.440879f;
bestparams.topbit = 1.680824f;
bestparams.distance = 0.02267573f;
bestparams.stmix = 0f;
                    break;
                    case 7:
// current score 32
bestparams.bias = 0.9842906f;
bestparams.pulsestrength = 2.772751f;
bestparams.topbit = 0f;
bestparams.distance = 0.4342486f;
bestparams.stmix = 1f;
                    break;
                }
            }
            if (chip == 'E') {
                switch (wave) {
                    case 3:
// current score 112
bestparams.bias = 0.9689716f;
bestparams.pulsestrength = 0f;
bestparams.topbit = 0f;
bestparams.distance = 3.088513f;
bestparams.stmix = 0.7588146f;
                    break;
                    case 5:
// current score 166
bestparams.bias = 0.9161022f;
bestparams.pulsestrength = 1.879311f;
bestparams.topbit = 1.0f;
bestparams.distance = 0.02331964f;
bestparams.stmix = 0f;
                    break;
                    case 6:
// current score 10
bestparams.bias = 0.879145f;
bestparams.pulsestrength = 1.30156f;
bestparams.topbit = 0f;
bestparams.distance = 0.006426161f;
bestparams.stmix = 0f;
                    break;
                    case 7:
// current score 8
bestparams.bias = 0.911324f;
bestparams.pulsestrength = 2.959469E-07f;
bestparams.topbit = 0f;
bestparams.distance = 4.148929E-06f;
bestparams.stmix = 1f;
                    break;
                }
            }
            if (chip == 'G') {
                switch (wave) {
                    case 3:
// current score 188
bestparams.bias = 0.9506974f;
bestparams.pulsestrength = 0f;
bestparams.topbit = 0f;
bestparams.distance = 2.104169f;
bestparams.stmix = 0.7887034f;
                    break;
                    case 5:
// current score 360
bestparams.bias = 0.8924618f;
bestparams.pulsestrength = 2.01122f;
bestparams.topbit = 1.0f;
bestparams.distance = 0.03133072f;
bestparams.stmix = 0f;
                    break;
                    case 6:
// current score 668
bestparams.bias = 0.8952018f;
bestparams.pulsestrength = 2.213601f;
bestparams.topbit = 1.705941f;
bestparams.distance = 0.01260567f;
bestparams.stmix = 0f;
                    break;
                    case 7:
// current score 12
bestparams.bias = 0.9527834f;
bestparams.pulsestrength = 1.794777f;
bestparams.topbit = 0f;
bestparams.distance = 0.09806272f;
bestparams.stmix = 0.7752482f;
                    break;
                }
            }
            if (chip == 'V') {
                switch (wave) {
                    case 3:
// current score 5546
bestparams.bias = 0.9781665f;
bestparams.pulsestrength = 0f;
bestparams.topbit = 0.9899469f;
bestparams.distance = 8.077209f;
bestparams.stmix = 0.8226412f;
                    break;
                    case 5:
// current score 628
bestparams.bias = 0.9236207f;
bestparams.pulsestrength = 2.19129f;
bestparams.topbit = 1f;
bestparams.distance = 0.1108298f;
bestparams.stmix = 0f;
                    break;
                    case 6:
// current score 593
bestparams.bias = 0.9248214f;
bestparams.pulsestrength = 2.232846f;
bestparams.topbit = 0.9491023f;
bestparams.distance = 0.1313893f;
bestparams.stmix = 0f;
                    break;
                    case 7:
// current score 168
bestparams.bias = 0.9845552f;
bestparams.pulsestrength = 1.381085f;
bestparams.topbit = 0.9621315f;
bestparams.distance = 1.699522f;
bestparams.stmix = 1f;
                    break;
                }
            }
            if (chip == 'W') {
                switch (wave) {
                    case 3:
// current score 315
bestparams.bias = 0.9686383f;
bestparams.pulsestrength = 0f;
bestparams.topbit = 0.9955494f;
bestparams.distance = 2.141501f;
bestparams.stmix = 0.9635284f;
                    break;
                    case 5:
// current score 784
bestparams.bias = 0.9069195f;
bestparams.pulsestrength = 2.203437f;
bestparams.topbit = 1f;
bestparams.distance = 0.129717f;
bestparams.stmix = 0f;
                    break;
                    case 6:
// current score 757
bestparams.bias = 0.9075199f;
bestparams.pulsestrength = 2.180424f;
bestparams.topbit = 0.9760311f;
bestparams.distance = 0.1208313f;
bestparams.stmix = 0f;
                    break;
                    case 7:
// current score 211
bestparams.bias = 0.9882526f;
bestparams.pulsestrength = 1.736355f;
bestparams.topbit = 0.9395381f;
bestparams.distance = 2.698372f;
bestparams.stmix = 1f;
                    break;
                }
            }

            var bestscore = bestparams.Score(wave, reference, true, 4096 * 255);
            Console.Write("# initial score {0}\n\n", bestscore);
            
            var p = new Parameters();
            while (true) {
                var changed = false;
                while (! changed) {
                    foreach (var el in new string[] { "bias", "pulsestrength", "topbit", "distance", "stmix" }) {
                        var field = typeof(Parameters).GetField(el);
                        var oldValue = (float) field.GetValue(bestparams);
                        var newValue = oldValue;
                        if (random.NextDouble() > 0.5) {
                            newValue *= GetRandomValue();
                            if (el == "stmix") {
                                if (newValue > 1f)
                                    newValue = 1f;
                            }
                        }

                        field.SetValue(p, newValue);
                        changed = changed || oldValue != newValue;
                    }
                }
                var score = p.Score(wave, reference, false, bestscore);
                /* accept if improvement */
                if (score <= bestscore) {
                    bestparams = p;
                    p = new Parameters();
                    bestscore = score;
                    Console.Write("# current score {0}\n{1}\n\n", score, bestparams);
                }
            }
        }

        private static int[] ReadChip(int wave, char chip) {
            Console.WriteLine("Reading chip: {0}", chip);
            var result = new int[4096];
            var i = 0;
            foreach (var line in
                File.ReadAllLines(string.Format("sidwaves/WAVE{0:X}.CSV", wave))
            ) {
                var values = line.Split(',');
                result[i ++] = Convert.ToInt32(values[chip - 'A']);
            }
            return result;
        }
        
        public static void Main(string[] args) {
            var wave = int.Parse(args[0]);
            Debug.Assert(wave == 3 || wave == 5 || wave == 6 || wave == 7);

            var chip = char.Parse(args[1]);
            Debug.Assert(chip >= 'A' && chip <= 'Z');

            var reference = ReadChip(wave, chip);
            Optimize(reference, wave, chip);
        }
    }
}
