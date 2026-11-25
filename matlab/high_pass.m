fs = 12000;
N = 12000;

Nb = 1550;
f0 = 1000 / (fs / 2);
b = fir1(Nb, f0, 'high');

% Print filter coefficients (float64_t)
fprintf('%.15g, ', b);
fprintf('\n');

% Print filter coefficients (q15_t)
b_q15 = int16(round(b * 32768));
b_q15(b_q15 > 32767) = 32767;
b_q15(b_q15 < -32768) = -32768;
fprintf('%d, ', b_q15);
fprintf('\n');

% Frequency characteristics
freqz(b, 1, N, fs);

% Verification
ts = 1 / fs;
t = (0:N-1)*ts;
df = (fs/N);
f = (0:N-1)*df;

X = 0;
for n = 1:79
    X = X + sin(2*pi*n*50*t);
end
% 16-bit uniform quantization
X = round((X + 1)*32767.5)/32767.5 - 1;

figure;
plot(t, X);
xlabel('Time (s)');
ylabel('Amplitude');
title('Signal');
grid on;

XF = filter(b, 1, X);

figure;
plot(t, XF);
xlabel('Time (s)');
ylabel('Amplitude');
title('Signal - Filtered');
grid on;

Y = 2 * abs(fft(X)) / N;

figure;
plot(f, Y);
ylim([0.0 1.0]);
xlabel("Freq (Hz)");
ylabel("Amplitude");
title('Spectrum');
grid on;

YF = 2 * abs(fft(XF)) / N;

figure;
plot(f, YF);
ylim([0.0 1.0]);
xlabel("Freq (Hz)");
ylabel("Amplitude");
title('Spectrum - Filtered');
grid on;
