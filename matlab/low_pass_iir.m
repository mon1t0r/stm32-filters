fs = 12000;
N = 12000;

Nb = 20;
Rp = 1;
f0 = 1000 / (fs / 2);
[z, p, k] = cheby1(Nb, Rp, f0, 'low');

[sos, g] = zp2sos(z, p, k);
sos(1, 1:3) = sos(1, 1:3) * g;

% Convert coefficients to arm_biquad_cascade_df1_init_q15 format
num_stages = size(sos, 1);

coeffs = zeros(1, 5 * num_stages, 'single');
for i = 1:num_stages

    b0 = sos(i, 1);
    b1 = sos(i, 2);
    b2 = sos(i, 3);

    a1 = sos(i, 5);
    a2 = sos(i, 6);

    coeffs(5*i-4:5*i) = single([b0, b1, b2, -a1, -a2]);

end

% Print coefficients
fprintf('Number of stages: %d\n', num_stages);
fprintf('Length: %d\n', length(coeffs));

fprintf('\nCoefs:\n');
fprintf('%d, ', coeffs);
fprintf('\n');

% Frequency characteristics
freqz(sos, N, fs);

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

XF = sosfilt(sos, X);

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
