/**
 * ClipSampler v2.6.1 — Motor de Interacción y Audio Web
 * Implementa síntesis en tiempo real con Web Audio API, emulador de 16 pads MPC,
 * visualizador de onda interactivo y pasarela de pago PayPal.
 */

document.addEventListener('DOMContentLoaded', () => {
  // =========================================================================
  // 1. MOTOR DE AUDIO WEB (SÍNTESIS EN TIEMPO REAL PARA DEMOS)
  // =========================================================================
  let audioCtx = null;

  function initAudioContext() {
    if (!audioCtx) {
      const AudioContextClass = window.AudioContext || window.webkitAudioContext;
      audioCtx = new AudioContextClass();
    }
    if (audioCtx.state === 'suspended') {
      audioCtx.resume();
    }
  }

  // Frecuencias para notas de C1 (36) a D#2 (51) transpuestas a rango audible musical
  const noteFrequencies = {
    36: 130.81, // C3
    37: 138.59, // C#3
    38: 146.83, // D3
    39: 155.56, // D#3
    40: 164.81, // E3
    41: 174.61, // F3
    42: 185.00, // F#3
    43: 196.00, // G3
    44: 207.65, // G#3
    45: 220.00, // A3
    46: 233.08, // A#3
    47: 246.94, // B3
    48: 261.63, // C4
    49: 277.18, // C#4
    50: 293.66, // D4
    51: 311.13  // D#4
  };

  /** Dispara un sonido sintetizado con textura de muestra analógica rebanada */
  function playSampleSlice(midiNote, type = 'slice') {
    initAudioContext();
    if (!audioCtx) return;

    const freq = noteFrequencies[midiNote] || 220.0;
    const now = audioCtx.currentTime;

    const osc = audioCtx.createOscillator();
    const gainNode = audioCtx.createGain();
    const filter = audioCtx.createBiquadFilter();

    // Filtro analógico 12-bit
    filter.type = 'lowpass';
    filter.frequency.setValueAtTime(1800, now);
    filter.frequency.exponentialRampToValueAtTime(350, now + 0.35);

    // Tipos de onda según la nota para emular batería, bajo y melodía
    if (midiNote === 36) {
      // Emulación Kick
      osc.type = 'sine';
      osc.frequency.setValueAtTime(120, now);
      osc.frequency.exponentialRampToValueAtTime(38, now + 0.18);
      gainNode.gain.setValueAtTime(0.85, now);
      gainNode.gain.exponentialRampToValueAtTime(0.001, now + 0.28);
    } else if (midiNote === 37) {
      // Emulación Snare / Transiente
      osc.type = 'triangle';
      osc.frequency.setValueAtTime(240, now);
      osc.frequency.exponentialRampToValueAtTime(110, now + 0.12);
      gainNode.gain.setValueAtTime(0.7, now);
      gainNode.gain.exponentialRampToValueAtTime(0.001, now + 0.22);
    } else if (midiNote >= 42 && midiNote <= 44) {
      // Emulación 808 / Sub
      osc.type = 'sine';
      osc.frequency.setValueAtTime(freq * 0.5, now);
      gainNode.gain.setValueAtTime(0.75, now);
      gainNode.gain.exponentialRampToValueAtTime(0.001, now + 0.5);
    } else {
      // Rodaja armónica / Vocal Chop / Acorde
      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(freq, now);
      gainNode.gain.setValueAtTime(0.45, now);
      gainNode.gain.exponentialRampToValueAtTime(0.001, now + 0.38);
    }

    osc.connect(filter);
    filter.connect(gainNode);
    gainNode.connect(audioCtx.destination);

    osc.start(now);
    osc.stop(now + 0.5);

    // Efecto visual en la forma de onda
    triggerWaveformBurst();
  }

  // =========================================================================
  // 2. VISUALIZADOR DE ONDA EN EL HERO (CANVAS 2D)
  // =========================================================================
  const canvas = document.getElementById('waveformCanvas');
  const playhead = document.getElementById('playhead');
  let ctx2d = null;
  let wavePoints = [];
  let isDemoPlaying = false;
  let demoInterval = null;

  if (canvas) {
    ctx2d = canvas.getContext('2d');
    generateStaticWaveform();
    drawWaveform();
  }

  function generateStaticWaveform() {
    wavePoints = [];
    const count = 140;
    for (let i = 0; i < count; i++) {
      // Simula picos de transientes rítmicos de un loop de sample
      const phase = (i % 18) / 18;
      const decay = Math.exp(-phase * 3.5);
      const rand = (Math.random() * 0.35 + 0.65);
      const amp = Math.sin(i * 0.15) * 0.25 + decay * 0.75 * rand;
      wavePoints.push(Math.min(1.0, Math.max(0.08, amp)));
    }
  }

  function drawWaveform(highlightIndex = -1) {
    if (!ctx2d || !canvas) return;

    const w = canvas.width;
    const h = canvas.height;
    const midY = h / 2;

    ctx2d.clearRect(0, 0, w, h);

    // Fondo sutil del canvas
    ctx2d.fillStyle = '#040811';
    ctx2d.fillRect(0, 0, w, h);

    // Líneas divisorias de rodajas (16 rodajas)
    const numSlices = 16;
    ctx2d.strokeStyle = 'rgba(56, 189, 248, 0.2)';
    ctx2d.lineWidth = 1;
    ctx2d.setLineDash([3, 3]);

    for (let s = 1; s < numSlices; s++) {
      const sliceX = (w / numSlices) * s;
      ctx2d.beginPath();
      ctx2d.moveTo(sliceX, 0);
      ctx2d.lineTo(sliceX, h);
      ctx2d.stroke();
    }
    ctx2d.setLineDash([]);

    // Dibujo de barras de forma de onda estéreo simétrica
    const barWidth = (w / wavePoints.length) - 1.5;
    for (let i = 0; i < wavePoints.length; i++) {
      const x = (w / wavePoints.length) * i;
      const barH = wavePoints[i] * (h * 0.42);

      const grad = ctx2d.createLinearGradient(x, midY - barH, x, midY + barH);
      if (highlightIndex >= 0 && Math.abs(i - highlightIndex) < 8) {
        grad.addColorStop(0, '#ffffff');
        grad.addColorStop(0.5, '#38bdf8');
        grad.addColorStop(1, '#0284c7');
      } else {
        grad.addColorStop(0, '#38bdf8');
        grad.addColorStop(0.5, '#0284c7');
        grad.addColorStop(1, '#0c4a6e');
      }

      ctx2d.fillStyle = grad;
      ctx2d.fillRect(x, midY - barH, barWidth, barH * 2);
    }
  }

  let auroraPulseEnergy = 0;

  function triggerWaveformBurst() {
    auroraPulseEnergy = 1.0;
    const burstIdx = Math.floor(Math.random() * wavePoints.length);
    drawWaveform(burstIdx);
    setTimeout(() => drawWaveform(-1), 160);
  }

  // =========================================================================
  // 3. CONTROLADOR DE PADS MPC (HERO + SECCIÓN 16 PADS)
  // =========================================================================
  const heroPads = document.querySelectorAll('#heroPadsGrid .pad-btn');
  const mpc16Pads = document.querySelectorAll('#mpc16Grid .mpc-pad');
  const lcdVoiceInfo = document.getElementById('lcdVoiceInfo');
  const lcdSliceInfo = document.getElementById('lcdSliceInfo');

  function triggerPadAction(padElement, note, noteName, padLabel) {
    padElement.classList.add('pad-active');
    setTimeout(() => padElement.classList.remove('pad-active'), 120);

    playSampleSlice(parseInt(note, 10));

    if (lcdVoiceInfo) lcdVoiceInfo.textContent = `VOZ: DISPARADA (${noteName})`;
    if (lcdSliceInfo) lcdSliceInfo.textContent = `RODAJA: ${padLabel || noteName} [MIDI ${note}]`;

    // Mueve el cabezal visual proporcional a la nota
    if (playhead && canvas) {
      const norm = ((parseInt(note, 10) - 36) / 16);
      playhead.style.left = `${Math.min(95, Math.max(2, norm * 100))}%`;
    }
  }

  // Clicks en pads del Hero
  heroPads.forEach(pad => {
    pad.addEventListener('click', () => {
      const note = pad.getAttribute('data-note');
      const name = pad.getAttribute('data-name');
      const small = pad.querySelector('small')?.textContent || '';
      triggerPadAction(pad, note, name, small);
    });
  });

  // Clicks en la cuadrícula de 16 pads MPC
  mpc16Pads.forEach(pad => {
    pad.addEventListener('click', () => {
      const note = pad.getAttribute('data-note');
      const noteName = pad.querySelector('.pad-note')?.textContent || 'C1';
      const padNum = pad.querySelector('.pad-num')?.textContent || 'PAD';
      triggerPadAction(pad, note, noteName, padNum);
    });
  });

  // Mapeo por teclado de computadora (1-4, Q-R, A-F, Z-V)
  const keyToPadMap = {
    '1': 48, '2': 49, '3': 50, '4': 51,
    'q': 44, 'w': 45, 'e': 46, 'r': 47,
    'a': 40, 's': 41, 'd': 42, 'f': 43,
    'z': 36, 'x': 37, 'c': 38, 'v': 39
  };

  window.addEventListener('keydown', (e) => {
    // Ignora si el usuario está escribiendo en un input
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;

    const key = e.key.toLowerCase();
    if (keyToPadMap[key]) {
      const note = keyToPadMap[key];
      const targetPad = document.querySelector(`#mpc16Grid .mpc-pad[data-key="${key}"]`);
      if (targetPad) {
        const noteName = targetPad.querySelector('.pad-note')?.textContent || '';
        const padNum = targetPad.querySelector('.pad-num')?.textContent || '';
        triggerPadAction(targetPad, note, noteName, padNum);
      }
    }
  });

  // =========================================================================
  // 4. BOTÓN "TOCAR DEMO" DEL HERO (SECUENCIA AUTOMÁTICA)
  // =========================================================================
  const btnPluginPlay = document.getElementById('btnPluginPlay');
  const playBtnText = document.getElementById('playBtnText');

  if (btnPluginPlay) {
    const demoSequence = [
      { note: 36, delay: 0 },
      { note: 38, delay: 240 },
      { note: 37, delay: 480 },
      { note: 40, delay: 720 },
      { note: 36, delay: 960 },
      { note: 42, delay: 1200 },
      { note: 37, delay: 1440 },
      { note: 45, delay: 1680 }
    ];

    btnPluginPlay.addEventListener('click', () => {
      initAudioContext();
      if (isDemoPlaying) {
        stopHeroDemo();
      } else {
        startHeroDemo();
      }
    });

    function startHeroDemo() {
      isDemoPlaying = true;
      if (playBtnText) playBtnText.textContent = 'Detener Demo';
      btnPluginPlay.style.background = '#dc2626';

      let step = 0;
      demoInterval = setInterval(() => {
        const item = demoSequence[step % demoSequence.length];
        playSampleSlice(item.note);

        if (playhead) {
          const pct = ((step % demoSequence.length) / demoSequence.length) * 100;
          playhead.style.left = `${pct}%`;
        }

        step++;
        if (step >= demoSequence.length * 4) { // 4 ciclos
          stopHeroDemo();
        }
      }, 250);
    }

    function stopHeroDemo() {
      isDemoPlaying = false;
      if (demoInterval) clearInterval(demoInterval);
      if (playBtnText) playBtnText.textContent = 'Tocar Demo';
      btnPluginPlay.style.background = '#0284c7';
      if (playhead) playhead.style.left = '0%';
    }
  }

  // =========================================================================
  // 5. AUDIO SHOWCASE PLAYERS (LO-FI, TRAP, REGGAETÓN, HOUSE)
  // =========================================================================
  const audioButtons = document.querySelectorAll('.btn-audio-play');
  let currentActiveDemo = null;
  let demoTimer = null;

  audioButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      initAudioContext();
      const genre = btn.getAttribute('data-demo');

      if (currentActiveDemo === genre) {
        stopAllGenreDemos();
      } else {
        stopAllGenreDemos();
        startGenreDemo(genre, btn);
      }
    });
  });

  function startGenreDemo(genre, btn) {
    currentActiveDemo = genre;
    btn.innerHTML = `<svg viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/></svg>`;

    const progressFill = document.getElementById(`progress${genre.charAt(0).toUpperCase() + genre.slice(1)}`);
    let progress = 0;

    // Rondas sintéticas por género
    const notesByGenre = {
      lofi: [36, 40, 37, 45, 36, 41, 37, 43],
      trap: [36, 42, 37, 46, 36, 42, 37, 48],
      reggaeton: [36, 37, 36, 37, 36, 37, 36, 37],
      house: [36, 44, 37, 44, 36, 44, 37, 44]
    };

    const notes = notesByGenre[genre] || [36, 38, 37, 40];
    let noteIdx = 0;

    demoTimer = setInterval(() => {
      playSampleSlice(notes[noteIdx % notes.length]);
      noteIdx++;

      progress += 5;
      if (progressFill) progressFill.style.width = `${progress}%`;

      if (progress >= 100) {
        stopAllGenreDemos();
      }
    }, 220);
  }

  function stopAllGenreDemos() {
    if (demoTimer) clearInterval(demoTimer);
    currentActiveDemo = null;

    audioButtons.forEach(b => {
      b.innerHTML = `<svg viewBox="0 0 24 24" fill="currentColor"><polygon points="6 4 20 12 6 20 6 4"/></svg>`;
    });

    document.querySelectorAll('.audio-progress-fill').forEach(fill => {
      fill.style.width = '0%';
    });
  }

  // =========================================================================
  // 6. MODAL DE CHECKOUT PAYPAL DIRECTO
  // =========================================================================
  const modal = document.getElementById('checkoutModal');
  const btnOpenCheckout = document.getElementById('btnOpenCheckout');
  const btnCloseModal = document.getElementById('btnCloseModal');
  const btnCopyEmail = document.getElementById('btnCopyEmail');
  const copyBtnLabel = document.getElementById('copyBtnLabel');
  const paypalEmailText = document.getElementById('paypalEmailText');

  // Abre el modal desde los botones de compra
  const buyButtons = document.querySelectorAll('a[href="#precio"], #btnOpenCheckout');
  buyButtons.forEach(btn => {
    btn.addEventListener('click', (e) => {
      if (btn.id === 'btnOpenCheckout' || btn.classList.contains('btn-primary')) {
        e.preventDefault();
        openModal();
      }
    });
  });

  function openModal() {
    if (modal) {
      modal.classList.add('is-open');
      modal.setAttribute('aria-hidden', 'false');
      document.body.style.overflow = 'hidden';
      const input = document.getElementById('buyerEmail');
      if (input) input.focus();
    }
  }

  function closeModal() {
    if (modal) {
      modal.classList.remove('is-open');
      modal.setAttribute('aria-hidden', 'true');
      document.body.style.overflow = '';
    }
  }

  if (btnCloseModal) btnCloseModal.addEventListener('click', closeModal);

  if (modal) {
    modal.addEventListener('click', (e) => {
      if (e.target === modal) closeModal();
    });
  }

  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && modal?.classList.contains('is-open')) {
      closeModal();
    }
  });

  // Copiar correo de PayPal con un clic
  if (btnCopyEmail && paypalEmailText) {
    btnCopyEmail.addEventListener('click', () => {
      const email = paypalEmailText.textContent.trim();
      navigator.clipboard.writeText(email).then(() => {
        if (copyBtnLabel) copyBtnLabel.textContent = '¡Copiado!';
        btnCopyEmail.style.background = '#059669';
        setTimeout(() => {
          if (copyBtnLabel) copyBtnLabel.textContent = 'Copiar';
          btnCopyEmail.style.background = '';
        }, 2200);
      });
    });
  }

  // =========================================================================
  // 7. CUENTA REGRESIVA DE LA OFERTA (12 HORAS DINÁMICAS)
  // =========================================================================
  const timerHours = document.getElementById('timerHours');
  const timerMins = document.getElementById('timerMins');
  const timerSecs = document.getElementById('timerSecs');

  let totalSeconds = (12 * 3600) + (45 * 60) + 30;

  function updateCountdown() {
    if (totalSeconds <= 0) {
      totalSeconds = 24 * 3600; // Reinicia el ciclo
    }
    totalSeconds--;

    const h = Math.floor(totalSeconds / 3600);
    const m = Math.floor((totalSeconds % 3600) / 60);
    const s = totalSeconds % 60;

    if (timerHours) timerHours.textContent = String(h).padStart(2, '0');
    if (timerMins) timerMins.textContent = String(m).padStart(2, '0');
    if (timerSecs) timerSecs.textContent = String(s).padStart(2, '0');
  }

  setInterval(updateCountdown, 1000);

  // =========================================================================
  // 8. MENÚ MÓVIL RESPONSIVO
  // =========================================================================
  const mobileToggle = document.getElementById('mobileToggle');
  const navLinks = document.getElementById('navLinks');

  if (mobileToggle && navLinks) {
    mobileToggle.addEventListener('click', () => {
      const isOpen = navLinks.classList.toggle('is-open');
      mobileToggle.setAttribute('aria-expanded', String(isOpen));
    });

    navLinks.querySelectorAll('a').forEach(link => {
      link.addEventListener('click', () => {
        navLinks.classList.remove('is-open');
        mobileToggle.setAttribute('aria-expanded', 'false');
      });
    });
  }

  // =========================================================================
  // 9. PESTAÑAS DEL HERO: 3 CAPTURAS REALES v2.6.1 VS SIMULADOR INTERACTIVO
  // =========================================================================
  const allHeroTabs = document.querySelectorAll('.window-tabs .win-tab-btn');
  const tabBtnInteractive = document.getElementById('tabBtnInteractive');
  const viewRealImage = document.getElementById('viewRealImage');
  const viewInteractive = document.getElementById('viewInteractive');
  const heroMainImg = document.getElementById('heroMainImg');
  const heroCaptionText = document.getElementById('heroCaptionText');
  const btnHeroAudioQuick = document.getElementById('btnHeroAudioQuick');

  if (allHeroTabs.length > 0 && viewRealImage && viewInteractive) {
    allHeroTabs.forEach((tab) => {
      tab.addEventListener('click', () => {
        allHeroTabs.forEach((t) => {
          t.classList.remove('active');
          t.setAttribute('aria-selected', 'false');
        });
        tab.classList.add('active');
        tab.setAttribute('aria-selected', 'true');

        if (tab === tabBtnInteractive) {
          viewInteractive.classList.add('active');
          viewRealImage.classList.remove('active');
          if (typeof drawWaveform === 'function') {
            drawWaveform();
          }
        } else {
          viewRealImage.classList.add('active');
          viewInteractive.classList.remove('active');
          const imgPath = tab.getAttribute('data-img');
          const capText = tab.getAttribute('data-caption');
          if (heroMainImg && imgPath) {
            heroMainImg.src = imgPath;
            heroMainImg.alt = capText || 'Captura real del plugin ClipSampler v2.6.1';
          }
          if (heroCaptionText && capText) {
            heroCaptionText.textContent = capText;
          }
        }
      });
    });
  }

  // Botón de reproducción rápida de audio en la barra del hero
  if (btnHeroAudioQuick) {
    btnHeroAudioQuick.addEventListener('click', (e) => {
      e.stopPropagation();
      initAudioContext();
      // Reproduce una secuencia corta de acordes/chops demostrativos
      const sampleNotes = [36, 40, 43, 47, 48];
      sampleNotes.forEach((note, idx) => {
        setTimeout(() => {
          playSampleSlice(note);
        }, idx * 110);
      });
      btnHeroAudioQuick.classList.add('playing');
      setTimeout(() => {
        btnHeroAudioQuick.classList.remove('playing');
      }, 700);
    });
  }

  // =========================================================================
  // 10. VISOR LIGHTBOX PARA AMPLIAR CAPTURAS DEL SOFTWARE EN HD
  // =========================================================================
  const imageLightbox = document.getElementById('imageLightbox');
  const lightboxImg = document.getElementById('lightboxImg');
  const lightboxCaption = document.getElementById('lightboxCaption');
  const btnCloseLightbox = document.getElementById('btnCloseLightbox');

  function openLightbox(imgSrc, captionText) {
    if (!imageLightbox || !lightboxImg) return;
    lightboxImg.src = imgSrc;
    lightboxImg.alt = captionText || 'Captura de pantalla de ClipSampler v2.6.1';
    if (lightboxCaption) {
      lightboxCaption.textContent = captionText || 'ClipSampler v2.6.1 - Software Real';
    }
    imageLightbox.classList.add('is-open');
    imageLightbox.setAttribute('aria-hidden', 'false');
    document.body.style.overflow = 'hidden';
  }

  function closeLightbox() {
    if (!imageLightbox) return;
    imageLightbox.classList.remove('is-open');
    imageLightbox.setAttribute('aria-hidden', 'true');
    document.body.style.overflow = '';
    if (lightboxImg) lightboxImg.src = '';
  }

  if (btnCloseLightbox) {
    btnCloseLightbox.addEventListener('click', closeLightbox);
  }

  if (imageLightbox) {
    imageLightbox.addEventListener('click', (e) => {
      if (e.target === imageLightbox) {
        closeLightbox();
      }
    });
  }

  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && imageLightbox?.classList.contains('is-open')) {
      closeLightbox();
    }
  });

  // Activar triggers de zoom en la imagen del hero
  const heroZoomTrigger = document.getElementById('heroZoomTrigger');
  if (heroZoomTrigger && heroMainImg) {
    const triggerHeroZoom = () => {
      openLightbox(
        heroMainImg.getAttribute('src'),
        heroCaptionText ? heroCaptionText.textContent : 'ClipSampler v2.6.1 — Interfaz Principal en Acción'
      );
    };
    heroZoomTrigger.addEventListener('click', triggerHeroZoom);
    heroMainImg.addEventListener('click', triggerHeroZoom);
  }

  // Activar triggers en todas las tarjetas de la galería
  const galleryContainers = document.querySelectorAll('.gallery-img-container');
  galleryContainers.forEach(container => {
    const handler = () => {
      const fullSrc = container.getAttribute('data-full');
      const img = container.querySelector('img');
      const caption = img ? img.getAttribute('alt') : 'ClipSampler v2.6.1';
      if (fullSrc) {
        openLightbox(fullSrc, caption);
      }
    };

    container.addEventListener('click', handler);
    container.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault();
        handler();
      }
    });
  });

  // =========================================================================
  // 11. REACT BITS / ACETERNITY UI: FONDO DE ONDAS SINUSOIDALES REACTIVAS
  //     (AURORA / WAVES SHADER DETRÁS DEL PLUGIN)
  // =========================================================================
  const auroraCanvas = document.getElementById('auroraWavesCanvas');
  if (auroraCanvas) {
    const aCtx = auroraCanvas.getContext('2d');
    const prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    let width = 0;
    let height = 0;
    let mouseX = 0.5;
    let mouseY = 0.5;
    let targetMouseX = 0.5;
    let targetMouseY = 0.5;
    let time = 0;

    function resizeAuroraCanvas() {
      const rect = auroraCanvas.parentElement?.getBoundingClientRect();
      width = auroraCanvas.width = rect ? rect.width : window.innerWidth;
      height = auroraCanvas.height = rect ? rect.height : 650;
    }

    resizeAuroraCanvas();
    window.addEventListener('resize', resizeAuroraCanvas, { passive: true });

    window.addEventListener('mousemove', (e) => {
      targetMouseX = e.clientX / Math.max(1, window.innerWidth);
      targetMouseY = e.clientY / Math.max(1, window.innerHeight);
    }, { passive: true });

    const waveLayers = [
      { color: 'rgba(56, 189, 248, 0.22)', amp: 38, freq: 0.0045, speed: 0.018, yOffset: 0.42, lineWidth: 2.2 },
      { color: 'rgba(45, 212, 191, 0.18)', amp: 52, freq: 0.0034, speed: -0.014, yOffset: 0.50, lineWidth: 1.8 },
      { color: 'rgba(249, 115, 22, 0.15)', amp: 30, freq: 0.0058, speed: 0.022, yOffset: 0.58, lineWidth: 1.6 },
      { color: 'rgba(168, 85, 247, 0.16)', amp: 44, freq: 0.0040, speed: 0.012, yOffset: 0.64, lineWidth: 1.5 }
    ];

    function renderAuroraWaves() {
      if (!aCtx) return;
      aCtx.clearRect(0, 0, width, height);

      mouseX += (targetMouseX - mouseX) * 0.06;
      mouseY += (targetMouseY - mouseY) * 0.06;
      auroraPulseEnergy *= 0.92;

      // Resplandor Aurora Reactivo detrás de las ondas sinusoidales
      const glowX = width * (0.35 + mouseX * 0.35);
      const glowY = height * (0.3 + mouseY * 0.3);
      const auroraGrad = aCtx.createRadialGradient(glowX, glowY, 20, glowX, glowY, Math.max(width, height) * 0.55);
      auroraGrad.addColorStop(0, `rgba(56, 189, 248, ${0.12 + auroraPulseEnergy * 0.12})`);
      auroraGrad.addColorStop(0.45, `rgba(45, 212, 191, ${0.06 + auroraPulseEnergy * 0.06})`);
      auroraGrad.addColorStop(1, 'rgba(13, 13, 17, 0)');
      aCtx.fillStyle = auroraGrad;
      aCtx.fillRect(0, 0, width, height);

      // Ondas sinusoidales reactivas
      const dynamicBoost = 1 + auroraPulseEnergy * 0.85 + (mouseY - 0.5) * 0.25;
      for (let l = 0; l < waveLayers.length; l++) {
        const layer = waveLayers[l];
        aCtx.beginPath();
        aCtx.strokeStyle = layer.color;
        aCtx.lineWidth = layer.lineWidth + auroraPulseEnergy * 1.2;

        const baseY = height * layer.yOffset;
        for (let x = 0; x <= width; x += 8) {
          const normalizedX = x / Math.max(1, width);
          const envelope = Math.sin(normalizedX * Math.PI);
          const wave1 = Math.sin(x * layer.freq + time * layer.speed * 60 + mouseX * 3.0);
          const wave2 = Math.cos(x * (layer.freq * 1.7) - time * layer.speed * 38);
          const y = baseY + (wave1 * 0.68 + wave2 * 0.32) * layer.amp * envelope * dynamicBoost;
          if (x === 0) {
            aCtx.moveTo(x, y);
          } else {
            aCtx.lineTo(x, y);
          }
        }
        aCtx.stroke();
      }

      if (!prefersReducedMotion) {
        time += 0.016;
        requestAnimationFrame(renderAuroraWaves);
      }
    }

    renderAuroraWaves();
  }
});
