/**
 * ClipSampler — Motor de Interacción y Audio Web
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
  // 7. PROMOCIÓN DE 24 HORAS EN LA PRIMERA VISITA (DE $29 USD A $50 USD AL EXPIRAR)
  // =========================================================================
  const PROMO_STORAGE_KEY = 'clipsampler_first_visit_ts_v261';
  const PROMO_DURATION_MS = 24 * 60 * 60 * 1000; // Exactamente 24 horas

  const timerHours = document.getElementById('timerHours');
  const timerMins = document.getElementById('timerMins');
  const timerSecs = document.getElementById('timerSecs');

  let firstVisitTs = null;
  try {
    const stored = localStorage.getItem(PROMO_STORAGE_KEY);
    if (stored && !isNaN(parseInt(stored, 10))) {
      firstVisitTs = parseInt(stored, 10);
    } else {
      firstVisitTs = Date.now();
      localStorage.setItem(PROMO_STORAGE_KEY, String(firstVisitTs));
    }
  } catch (err) {
    // En caso de navegación privada con localStorage restringido
    firstVisitTs = Date.now();
  }

  let promoExpiredApplied = false;

  function applyExpiredPromoPrice() {
    if (promoExpiredApplied) return;
    promoExpiredApplied = true;

    const promoBadge = document.getElementById('promoBadge');
    const promoBarText = document.getElementById('promoBarText');
    const promoActionLink = document.getElementById('promoActionLink');
    const navBuyBtnText = document.getElementById('navBuyBtnText');
    const heroBuyBtnText = document.getElementById('heroBuyBtnText');
    const pricingSectionTag = document.getElementById('pricingSectionTag');
    const priceNumber = document.getElementById('priceNumber');
    const discountBadgeBox = document.getElementById('discountBadgeBox');
    const btnOpenCheckoutText = document.getElementById('btnOpenCheckoutText');
    const modalOriginalPriceRow = document.getElementById('modalOriginalPriceRow');
    const modalDiscountRow = document.getElementById('modalDiscountRow');
    const modalTotalAmount = document.getElementById('modalTotalAmount');
    const paypalAmountInput = document.getElementById('paypalAmountInput');
    const paypalItemNameInput = document.getElementById('paypalItemNameInput');
    const btnPaypalSubmitText = document.getElementById('btnPaypalSubmitText');

    if (promoBadge) promoBadge.textContent = 'OFERTA FINALIZADA';
    if (promoBarText) {
      promoBarText.innerHTML = 'Tu tiempo de promoción de 24 horas ha finalizado. Obtén <strong>ClipSampler</strong> (Instalador 100% en Español + Manual PDF) por <span class="price-highlight">$50 USD</span>';
    }
    if (promoActionLink) promoActionLink.textContent = 'Adquirir Licencia';
    if (navBuyBtnText) navBuyBtnText.textContent = 'Comprar Ahora • $50 USD';
    if (heroBuyBtnText) heroBuyBtnText.textContent = 'Comprar Licencia De Por Vida — $50 USD';
    if (pricingSectionTag) pricingSectionTag.textContent = 'LICENCIA DE POR VIDA OFICIAL';
    if (priceNumber) priceNumber.textContent = '50';
    if (discountBadgeBox) discountBadgeBox.style.display = 'none';
    if (btnOpenCheckoutText) btnOpenCheckoutText.textContent = 'Pagar $50 USD con PayPal';

    if (modalOriginalPriceRow) {
      modalOriginalPriceRow.innerHTML = '<span>Precio de lista oficial:</span><strong>$50.00 USD</strong>';
    }
    if (modalDiscountRow) modalDiscountRow.style.display = 'none';
    if (modalTotalAmount) modalTotalAmount.textContent = '$50.00 USD';
    if (paypalAmountInput) paypalAmountInput.value = '50.00';
    if (paypalItemNameInput) {
      paypalItemNameInput.value = 'ClipSampler - Licencia De Por Vida Oficial (Windows y Mac) - $50 USD';
    }
    if (btnPaypalSubmitText) {
      btnPaypalSubmitText.textContent = 'Proceder al Pago Seguro de $50 USD';
    }

    document.querySelectorAll('.dynamic-price-label').forEach((el) => {
      el.textContent = '$50 USD';
    });
  }

  function updateCountdown() {
    const elapsedMs = Date.now() - firstVisitTs;
    const remainingMs = PROMO_DURATION_MS - elapsedMs;

    if (remainingMs <= 0) {
      if (timerHours) timerHours.textContent = '00';
      if (timerMins) timerMins.textContent = '00';
      if (timerSecs) timerSecs.textContent = '00';
      applyExpiredPromoPrice();
      return;
    }

    const totalSeconds = Math.ceil(remainingMs / 1000);
    const h = Math.floor(totalSeconds / 3600);
    const m = Math.floor((totalSeconds % 3600) / 60);
    const s = totalSeconds % 60;

    if (timerHours) timerHours.textContent = String(h).padStart(2, '0');
    if (timerMins) timerMins.textContent = String(m).padStart(2, '0');
    if (timerSecs) timerSecs.textContent = String(s).padStart(2, '0');
  }

  updateCountdown();
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
  // 9. PESTAÑAS DEL HERO: 3 CAPTURAS REALES VS SIMULADOR INTERACTIVO
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
            heroMainImg.alt = capText || 'Captura real del plugin ClipSampler';
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
    lightboxImg.alt = captionText || 'Captura de pantalla de ClipSampler';
    if (lightboxCaption) {
      lightboxCaption.textContent = captionText || 'ClipSampler - Software Real';
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
        heroCaptionText ? heroCaptionText.textContent : 'ClipSampler — Interfaz Principal en Acción'
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
      const caption = img ? img.getAttribute('alt') : 'ClipSampler';
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

  // =========================================================================
  // 12. RÉPLICA INTERACTIVA 1:1 DEL PLUGIN REAL (VISOR 1 + VISOR 2 + REBANADOR MPC)
  // =========================================================================
  const csLivePlugin = document.getElementById('csLivePlugin');
  const csVisor1Canvas = document.getElementById('csVisor1Canvas');
  const csVisor2Canvas = document.getElementById('csVisor2Canvas');

  if (csLivePlugin && csVisor1Canvas && csVisor2Canvas) {
    const v1Ctx = csVisor1Canvas.getContext('2d');
    const v2Ctx = csVisor2Canvas.getContext('2d');

    const SLICE_NAMES = ['C1', 'C#1', 'D1', 'D#1', 'E1', 'F1', 'F#1', 'G1', 'G#1', 'A1', 'A#1'];
    // Posiciones iniciales exactas de las 11 rodajas como en la captura real
    const DEFAULT_BOUNDARIES = [
      0.000, 0.185, 0.252, 0.318, 0.380, 0.448,
      0.514, 0.580, 0.646, 0.713, 0.779, 1.000
    ];

    const state = {
      activeSlot: 2, // 1 = Visor 1 (Original), 2 = Visor 2 (Calor Analógico)
      mpcEnabled: true,
      syncBpm: false,
      reverse: false,
      halfSpeed: false,
      lofiIndex: 0, // 0%, 35%, 70%
      pitchShiftSt: 0, // semitonos adicionales
      visor2AiSt: 3, // +3 st IA
      filterCutoffHz: 18000,
      sampleDuration: 6.0,
      slot1: {
        boundaries: [...DEFAULT_BOUNDARIES],
        selectedSlice: -1,
        exactCutLocked: false,
        playheadNorm: -1,
        audioBuffer: null,
        reversedBuffer: null,
        peaksL: [],
        peaksR: []
      },
      slot2: {
        boundaries: [...DEFAULT_BOUNDARIES],
        selectedSlice: 1, // C#1 resaltado inicialmente o al hacer clic
        exactCutLocked: false,
        playheadNorm: -1,
        audioBuffer: null,
        reversedBuffer: null,
        peaksL: [],
        peaksR: []
      }
    };

    let activeSourceNode = null;
    let playheadAnimId = null;
    let playStartCtxTime = 0;
    let playDurationSec = 0;
    let playStartNorm = 0;
    let playEndNorm = 1;
    let playingSlotNum = 2;
    let autoDemoTimer = null;
    let isAutoDemoRunning = false;

    // Elementos de la interfaz del plugin
    const csLiveLed = document.getElementById('csLiveLed');
    const csStatusMsg = document.getElementById('csStatusMsg');
    const csRightsBadge = document.getElementById('csRightsBadge');
    const csSlotBtn1 = document.getElementById('csSlotBtn1');
    const csSlotBtn2 = document.getElementById('csSlotBtn2');
    const csVisorBox1 = document.getElementById('csVisorBox1');
    const csVisorBox2 = document.getElementById('csVisorBox2');
    const csVisor1Title = document.getElementById('csVisor1Title');
    const csVisor2Title = document.getElementById('csVisor2Title');
    const csVisor2Meta = document.getElementById('csVisor2Meta');
    const csActiveSlotIndicator = document.getElementById('csActiveSlotIndicator');
    const csKbButtons = document.querySelectorAll('#csKbKeys .cs-key-btn');

    // Genera envolvente de respaldo idéntica a la captura mientras se decodifica el MP3 real
    function buildFallbackPeaks(numPoints, isVisor2) {
      const pL = new Float32Array(numPoints);
      const pR = new Float32Array(numPoints);
      for (let i = 0; i < numPoints; i++) {
        const norm = i / numPoints;
        // En la captura real el sonido comienza en ~0.075 y termina en ~0.89
        if (norm < 0.074 || norm > 0.895) {
          pL[i] = 0.008;
          pR[i] = 0.008;
          continue;
        }
        const env = Math.sin(((norm - 0.074) / 0.821) * Math.PI);
        const transient = Math.abs(Math.sin(i * 0.43) * Math.cos(i * 0.19));
        const grit = Math.abs(Math.sin(i * 1.73));
        const boost = isVisor2 ? 1.28 : 0.72;
        pL[i] = Math.min(0.96, (0.15 + transient * 0.65 + grit * 0.2) * Math.pow(env, 0.45) * boost);
        pR[i] = Math.min(0.96, (0.15 + Math.abs(Math.cos(i * 0.37)) * 0.65 + grit * 0.2) * Math.pow(env, 0.45) * boost);
      }
      return { pL, pR };
    }

    const NUM_PEAKS = 580;
    const fb1 = buildFallbackPeaks(NUM_PEAKS, false);
    state.slot1.peaksL = fb1.pL;
    state.slot1.peaksR = fb1.pR;
    const fb2 = buildFallbackPeaks(NUM_PEAKS, true);
    state.slot2.peaksL = fb2.pL;
    state.slot2.peaksR = fb2.pR;

    function extractStereoPeaks(audioBuffer, count) {
      const pL = new Float32Array(count);
      const pR = new Float32Array(count);
      const ch0 = audioBuffer.getChannelData(0);
      const ch1 = audioBuffer.numberOfChannels > 1 ? audioBuffer.getChannelData(1) : ch0;
      const samplesPerBucket = Math.max(1, Math.floor(ch0.length / count));

      for (let i = 0; i < count; i++) {
        const start = i * samplesPerBucket;
        const end = Math.min(ch0.length, start + samplesPerBucket);
        let maxL = 0;
        let maxR = 0;
        for (let s = start; s < end; s += 2) {
          const vL = Math.abs(ch0[s]);
          const vR = Math.abs(ch1[s]);
          if (vL > maxL) maxL = vL;
          if (vR > maxR) maxR = vR;
        }
        pL[i] = Math.min(1.0, maxL);
        pR[i] = Math.min(1.0, maxR);
      }
      return { pL, pR };
    }

    function createReversedBuffer(ctx, buffer) {
      const rev = ctx.createBuffer(buffer.numberOfChannels, buffer.length, buffer.sampleRate);
      for (let c = 0; c < buffer.numberOfChannels; c++) {
        const src = buffer.getChannelData(c);
        const dst = rev.getChannelData(c);
        for (let i = 0, n = src.length; i < n; i++) {
          dst[i] = src[n - 1 - i];
        }
      }
      return rev;
    }

    function base64ToArrayBuffer(b64) {
      const binary = window.atob(b64);
      const len = binary.length;
      const bytes = new Uint8Array(len);
      for (let i = 0; i < len; i++) {
        bytes[i] = binary.charCodeAt(i);
      }
      return bytes.buffer;
    }

    function decodeBufferSafe(ctx, arrayBuf, onSuccess, onError) {
      if (!ctx || !arrayBuf) return;
      let handled = false;
      const handleSuccess = (decoded) => {
        if (!handled) {
          handled = true;
          if (onSuccess) onSuccess(decoded);
        }
      };
      const handleError = (err) => {
        if (!handled) {
          handled = true;
          if (onError) onError(err);
        }
      };
      try {
        const res = ctx.decodeAudioData(arrayBuf, handleSuccess, handleError);
        if (res && typeof res.then === 'function') {
          res.then(handleSuccess).catch(handleError);
        }
      } catch (e) {
        handleError(e);
      }
    }

    // Decodifica los dos audios reales embebidos en window.CLIPSAMPLER_AUDIO_SAMPLES
    function decodeEmbeddedRealSamples() {
      const samples = window.CLIPSAMPLER_AUDIO_SAMPLES;
      if (!samples) return;

      const OfflineCtx = window.OfflineAudioContext || window.webkitOfflineAudioContext;
      const decoderCtx = OfflineCtx ? new OfflineCtx(2, 44100, 44100) : (audioCtx || null);
      if (!decoderCtx) return;

      if (samples.visor1Base64 && !state.slot1.audioBuffer) {
        try {
          const buf1 = base64ToArrayBuffer(samples.visor1Base64);
          decodeBufferSafe(decoderCtx, buf1, (decoded) => {
            state.slot1.audioBuffer = decoded;
            state.slot1.reversedBuffer = createReversedBuffer(decoderCtx, decoded);
            state.sampleDuration = decoded.duration || 6.0;
            const peaks = extractStereoPeaks(decoded, NUM_PEAKS);
            state.slot1.peaksL = peaks.pL;
            state.slot1.peaksR = peaks.pR;
            drawVisor(1);
          });
        } catch (e) {}
      }

      if (samples.visor2Base64 && !state.slot2.audioBuffer) {
        try {
          const buf2 = base64ToArrayBuffer(samples.visor2Base64);
          decodeBufferSafe(decoderCtx, buf2, (decoded) => {
            state.slot2.audioBuffer = decoded;
            state.slot2.reversedBuffer = createReversedBuffer(decoderCtx, decoded);
            const peaks = extractStereoPeaks(decoded, NUM_PEAKS);
            state.slot2.peaksL = peaks.pL;
            state.slot2.peaksR = peaks.pR;
            drawVisor(2);
          });
        } catch (e) {}
      }
    }

    // Dibuja el Visor 1 (Azul/Cian) o el Visor 2 (Verde/Violeta) idéntico al software real
    function drawVisor(slotNum) {
      const canvasEl = slotNum === 1 ? csVisor1Canvas : csVisor2Canvas;
      const ctx = slotNum === 1 ? v1Ctx : v2Ctx;
      const slot = slotNum === 1 ? state.slot1 : state.slot2;
      if (!canvasEl || !ctx) return;

      const w = canvasEl.width;
      const h = canvasEl.height;
      const topBarH = 22;
      const waveAreaH = h - topBarH;

      ctx.clearRect(0, 0, w, h);

      // 1. Fondo oscuro de estudio
      ctx.fillStyle = '#050912';
      ctx.fillRect(0, 0, w, h);

      // 2. Cuadrícula sutil de fondo
      ctx.strokeStyle = 'rgba(56, 189, 248, 0.06)';
      ctx.lineWidth = 1;
      const gridCols = 24;
      for (let c = 1; c < gridCols; c++) {
        const gx = (w / gridCols) * c;
        ctx.beginPath();
        ctx.moveTo(gx, topBarH);
        ctx.lineTo(gx, h);
        ctx.stroke();
      }

      const centerL = topBarH + waveAreaH * 0.27;
      const centerR = topBarH + waveAreaH * 0.73;

      // Líneas centrales de canal L y R
      ctx.strokeStyle = slotNum === 1 ? 'rgba(56, 189, 248, 0.35)' : 'rgba(0, 230, 118, 0.3)';
      ctx.beginPath();
      ctx.moveTo(0, centerL);
      ctx.lineTo(w, centerL);
      ctx.stroke();

      ctx.strokeStyle = slotNum === 1 ? 'rgba(56, 189, 248, 0.35)' : 'rgba(167, 139, 250, 0.3)';
      ctx.beginPath();
      ctx.moveTo(0, centerR);
      ctx.lineTo(w, centerR);
      ctx.stroke();

      // 3. Resaltado de la rodaja seleccionada / activa
      const bounds = slot.boundaries;
      if (state.mpcEnabled && slot.selectedSlice >= 0 && slot.selectedSlice < SLICE_NAMES.length) {
        const sIdx = slot.selectedSlice;
        const x0 = bounds[sIdx] * w;
        const x1 = bounds[sIdx + 1] * w;
        ctx.fillStyle = slotNum === 1
          ? 'rgba(56, 189, 248, 0.14)'
          : 'rgba(0, 230, 118, 0.14)';
        ctx.fillRect(x0, topBarH, Math.max(2, x1 - x0), waveAreaH);
      }

      // 4. Dibujo de las dos ondas estéreo (Canal L arriba, Canal R abajo)
      const peaksL = slot.peaksL;
      const peaksR = slot.peaksR;
      const numPts = peaksL.length;
      const maxAmpH = waveAreaH * 0.22;

      for (let i = 0; i < numPts; i++) {
        const x = (i / numPts) * w;
        const normX = i / numPts;
        const inActiveSlice = state.mpcEnabled &&
          slot.selectedSlice >= 0 &&
          normX >= bounds[slot.selectedSlice] &&
          normX <= bounds[slot.selectedSlice + 1];

        const ampL = Math.max(0.01, peaksL[i]) * (slotNum === 1 ? maxAmpH * 0.82 : maxAmpH * 1.05);
        const ampR = Math.max(0.01, peaksR[i]) * (slotNum === 1 ? maxAmpH * 0.82 : maxAmpH * 1.05);

        if (slotNum === 1) {
          ctx.fillStyle = inActiveSlice ? '#7dd3fc' : '#22a6f2';
          ctx.fillRect(x, centerL - ampL, 1.6, ampL * 2);
          ctx.fillRect(x, centerR - ampR, 1.6, ampR * 2);
        } else {
          // Visor 2: Canal superior Verde Neón, Canal inferior Violeta Analógico (como en la imagen real)
          ctx.fillStyle = inActiveSlice ? '#6ee7b7' : '#10b981';
          ctx.fillRect(x, centerL - ampL, 1.6, ampL * 2);
          ctx.fillStyle = inActiveSlice ? '#c4b5fd' : '#8b5cf6';
          ctx.fillRect(x, centerR - ampR, 1.6, ampR * 2);
        }
      }

      // 5. Barra superior de notas (C1 a A#1) y líneas divisorias amarillas punteadas
      ctx.fillStyle = 'rgba(23, 33, 53, 0.94)';
      ctx.fillRect(0, 0, w, topBarH);
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
      ctx.beginPath();
      ctx.moveTo(0, topBarH);
      ctx.lineTo(w, topBarH);
      ctx.stroke();

      if (state.mpcEnabled) {
        ctx.font = 'bold 11px Inter, system-ui, sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';

        for (let s = 0; s < SLICE_NAMES.length; s++) {
          const xStart = bounds[s] * w;
          const xEnd = bounds[s + 1] * w;
          const midX = (xStart + xEnd) * 0.5;
          const isSelected = (slot.selectedSlice === s);

          if (isSelected) {
            ctx.fillStyle = 'rgba(250, 204, 21, 0.25)';
            ctx.fillRect(xStart, 0, xEnd - xStart, topBarH);
            ctx.fillStyle = '#fde047';
          } else {
            ctx.fillStyle = '#e2e8f0';
          }
          ctx.fillText(SLICE_NAMES[s], midX, topBarH * 0.52);

          // Línea amarilla punteada divisoria de cada corte
          if (s > 0) {
            ctx.save();
            ctx.strokeStyle = isSelected ? '#fde047' : '#eab308';
            ctx.lineWidth = 1.5;
            ctx.setLineDash([4, 3]);
            ctx.beginPath();
            ctx.moveTo(xStart, 0);
            ctx.lineTo(xStart, h);
            ctx.stroke();
            ctx.restore();
          }
        }
      }

      // 6. Cabezal de reproducción animado (Playhead Rojo brillante)
      if (slot.playheadNorm >= 0 && slot.playheadNorm <= 1) {
        const px = slot.playheadNorm * w;
        ctx.save();
        ctx.strokeStyle = '#ff2a55';
        ctx.lineWidth = 2.2;
        ctx.shadowColor = '#ff2a55';
        ctx.shadowBlur = 10;
        ctx.beginPath();
        ctx.moveTo(px, 0);
        ctx.lineTo(px, h);
        ctx.stroke();

        // Triángulo indicador superior del cabezal
        ctx.fillStyle = '#ff2a55';
        ctx.beginPath();
        ctx.moveTo(px - 6, 0);
        ctx.lineTo(px + 6, 0);
        ctx.lineTo(px, 9);
        ctx.closePath();
        ctx.fill();
        ctx.restore();
      }
    }

    function redrawBothVisors() {
      drawVisor(1);
      drawVisor(2);
    }

    function setActiveSlot(slotNum) {
      state.activeSlot = slotNum;
      if (csSlotBtn1 && csSlotBtn2) {
        csSlotBtn1.classList.toggle('active-slot1', slotNum === 1);
        csSlotBtn2.classList.toggle('active-slot2', slotNum === 2);
      }
      if (csVisorBox1 && csVisorBox2) {
        csVisorBox1.classList.toggle('is-active', slotNum === 1);
        csVisorBox2.classList.toggle('is-active', slotNum === 2);
      }
      if (csActiveSlotIndicator) {
        csActiveSlotIndicator.innerHTML = slotNum === 1
          ? 'Disparando: <strong style="color:#38bdf8">VISOR 1 (AUDIO ORIGINAL FREESOUND)</strong>'
          : 'Disparando: <strong style="color:#00e676">VISOR 2 (CALOR ANALÓGICO +3 TONOS IA)</strong>';
      }
      updateVisorTitles();
      redrawBothVisors();
    }

    function updateVisorTitles() {
      const s1 = state.slot1;
      const s2 = state.slot2;
      const dur = state.sampleDuration || 6.0;

      if (csVisor1Title) {
        if (s1.selectedSlice >= 0 && s1.exactCutLocked) {
          const t0 = (s1.boundaries[s1.selectedSlice] * dur).toFixed(2);
          const t1 = (s1.boundaries[s1.selectedSlice + 1] * dur).toFixed(2);
          csVisor1Title.textContent = `VISOR 1: AUDIO ORIGINAL (REBANADOR MPC C1-G2)  [✂ CORTE EXACTO: ${SLICE_NAMES[s1.selectedSlice]} (${t0}s - ${t1}s) | 2 CLICS = TODO]`;
        } else {
          csVisor1Title.textContent = 'VISOR 1: AUDIO ORIGINAL (REBANADOR MPC C1-G2)  [TODO EL AUDIO | 2 CLICS = CORTE]';
        }
      }

      if (csVisor2Title) {
        if (s2.selectedSlice >= 0 && s2.exactCutLocked) {
          const t0 = (s2.boundaries[s2.selectedSlice] * dur).toFixed(2);
          const t1 = (s2.boundaries[s2.selectedSlice + 1] * dur).toFixed(2);
          csVisor2Title.textContent = `VISOR 2: CALOR ANALÓGICO (TAPE WARMTH)  [✂ CORTE EXACTO: ${SLICE_NAMES[s2.selectedSlice]} (${t0}s - ${t1}s) | 2 CLICS = TODO]`;
        } else {
          csVisor2Title.textContent = 'VISOR 2: CALOR ANALÓGICO (TAPE WARMTH)  [TODO EL AUDIO | 2 CLICS = CORTE]';
        }
      }

      if (csVisor2Meta) {
        const totalSt = state.visor2AiSt + state.pitchShiftSt;
        const sign = totalSt >= 0 ? '+' : '';
        csVisor2Meta.textContent = `${sign}${totalSt} st IA | [LOCAL]  91.5 BPM | Tono: Gm | 6.0s`;
      }
    }

    function stopCurrentPlayback() {
      if (activeSourceNode) {
        try {
          activeSourceNode.stop();
          activeSourceNode.disconnect();
        } catch (e) {}
        activeSourceNode = null;
      }
      if (playheadAnimId) {
        cancelAnimationFrame(playheadAnimId);
        playheadAnimId = null;
      }
      state.slot1.playheadNorm = -1;
      state.slot2.playheadNorm = -1;
      if (csLiveLed) csLiveLed.classList.remove('led-active');
      csKbButtons.forEach(b => b.classList.remove('is-playing'));
      redrawBothVisors();
    }

    // Reproduce una rodaja específica (0..10) o el sample completo (-1) con audio real y animación de playhead
    function playVisorRegion(slotNum, sliceIdx, lockExactCut = false) {
      initAudioContext();
      if (!audioCtx) return;
      if (!state.slot1.audioBuffer || !state.slot2.audioBuffer) {
        decodeEmbeddedRealSamples();
      }

      stopCurrentPlayback();
      setActiveSlot(slotNum);

      const slot = slotNum === 1 ? state.slot1 : state.slot2;
      if (sliceIdx >= 0 && sliceIdx < SLICE_NAMES.length) {
        slot.selectedSlice = sliceIdx;
        if (lockExactCut) {
          slot.exactCutLocked = true;
        }
      } else {
        slot.selectedSlice = -1;
        slot.exactCutLocked = false;
      }
      updateVisorTitles();

      const startNorm = (sliceIdx >= 0 && sliceIdx < SLICE_NAMES.length)
        ? slot.boundaries[sliceIdx]
        : 0.0;
      const endNorm = (sliceIdx >= 0 && sliceIdx < SLICE_NAMES.length)
        ? slot.boundaries[sliceIdx + 1]
        : 1.0;

      // Ilumina la tecla correspondiente en el teclado inferior
      csKbButtons.forEach(btn => {
        const bSlice = parseInt(btn.getAttribute('data-slice'), 10);
        btn.classList.toggle('is-playing', bSlice === sliceIdx);
      });

      if (csLiveLed) csLiveLed.classList.add('led-active');
      auroraPulseEnergy = 1.0;

      const bufferToUse = state.reverse
        ? (slot.reversedBuffer || slot.audioBuffer)
        : slot.audioBuffer;

      let playbackRate = Math.pow(2, state.pitchShiftSt / 12);
      if (state.halfSpeed) playbackRate *= 0.5;
      if (state.syncBpm) playbackRate *= (130.0 / 91.5);

      if (bufferToUse) {
        const totalDur = bufferToUse.duration;
        const offsetSec = state.reverse
          ? (1.0 - endNorm) * totalDur
          : startNorm * totalDur;
        const sliceDurSec = Math.max(0.05, (endNorm - startNorm) * totalDur);

        const src = audioCtx.createBufferSource();
        src.buffer = bufferToUse;
        src.playbackRate.value = playbackRate;

        const filterNode = audioCtx.createBiquadFilter();
        filterNode.type = 'lowpass';
        const lofiCutoffs = [18500, 3200, 1400];
        filterNode.frequency.value = Math.min(state.filterCutoffHz, lofiCutoffs[state.lofiIndex] || 18500);

        const gainNode = audioCtx.createGain();
        const now = audioCtx.currentTime;
        const effectiveDur = sliceDurSec / playbackRate;
        gainNode.gain.setValueAtTime(0.01, now);
        gainNode.gain.linearRampToValueAtTime(0.95, now + 0.008);
        gainNode.gain.setValueAtTime(0.95, Math.max(now + 0.01, now + effectiveDur - 0.04));
        gainNode.gain.linearRampToValueAtTime(0.0001, now + effectiveDur);

        src.connect(filterNode);
        filterNode.connect(gainNode);
        gainNode.connect(audioCtx.destination);

        src.start(now, offsetSec, sliceDurSec);
        activeSourceNode = src;

        playingSlotNum = slotNum;
        playStartCtxTime = now;
        playDurationSec = effectiveDur;
        playStartNorm = startNorm;
        playEndNorm = endNorm;

        const animatePlayhead = () => {
          if (!activeSourceNode) return;
          const elapsed = audioCtx.currentTime - playStartCtxTime;
          const prog = Math.min(1.0, elapsed / Math.max(0.01, playDurationSec));
          const activeSlotObj = playingSlotNum === 1 ? state.slot1 : state.slot2;
          activeSlotObj.playheadNorm = state.reverse
            ? (playEndNorm - prog * (playEndNorm - playStartNorm))
            : (playStartNorm + prog * (playEndNorm - playStartNorm));

          drawVisor(playingSlotNum);

          if (prog < 1.0) {
            playheadAnimId = requestAnimationFrame(animatePlayhead);
          } else {
            activeSlotObj.playheadNorm = -1;
            activeSourceNode = null;
            if (csLiveLed) csLiveLed.classList.remove('led-active');
            csKbButtons.forEach(b => b.classList.remove('is-playing'));
            drawVisor(playingSlotNum);
          }
        };
        playheadAnimId = requestAnimationFrame(animatePlayhead);
      } else {
        // Respaldo inmediato si el navegador aún está decodificando
        playSampleSlice(36 + Math.max(0, sliceIdx));
      }

      if (csStatusMsg) {
        const visorLabel = slotNum === 1 ? 'Visor 1 (Original)' : 'Visor 2 (Calor Analógico)';
        if (sliceIdx >= 0) {
          const t0 = (startNorm * state.sampleDuration).toFixed(2);
          const t1 = (endNorm * state.sampleDuration).toFixed(2);
          csStatusMsg.textContent = `🔊 Reproduciendo rodaja ${SLICE_NAMES[sliceIdx]} (${t0}s – ${t1}s) en ${visorLabel}. Arrastra las líneas amarillas para ajustar el corte.`;
        } else {
          csStatusMsg.textContent = `🔊 Reproduciendo muestra completa (6.0s) en ${visorLabel}.`;
        }
      }
    }

    // Interacción con el mouse en ambos Canvases (Clic, Doble Clic y Arrastrar Líneas Amarillas)
    function attachVisorMouseHandlers(canvasEl, slotNum) {
      let draggingBoundaryIdx = -1;
      let hoveredBoundaryIdx = -1;

      function getNormX(evt) {
        const rect = canvasEl.getBoundingClientRect();
        return Math.max(0, Math.min(1, (evt.clientX - rect.left) / Math.max(1, rect.width)));
      }

      function findNearbyBoundary(normX, boundaries) {
        if (!state.mpcEnabled) return -1;
        const threshold = 0.012; // ~14px de tolerancia
        for (let i = 1; i < boundaries.length - 1; i++) {
          if (Math.abs(normX - boundaries[i]) < threshold) {
            return i;
          }
        }
        return -1;
      }

      function findSliceAtNorm(normX, boundaries) {
        for (let i = 0; i < SLICE_NAMES.length; i++) {
          if (normX >= boundaries[i] && normX <= boundaries[i + 1]) {
            return i;
          }
        }
        return 0;
      }

      canvasEl.addEventListener('mousemove', (e) => {
        const slot = slotNum === 1 ? state.slot1 : state.slot2;
        const normX = getNormX(e);

        if (draggingBoundaryIdx > 0) {
          const minX = slot.boundaries[draggingBoundaryIdx - 1] + 0.025;
          const maxX = slot.boundaries[draggingBoundaryIdx + 1] - 0.025;
          slot.boundaries[draggingBoundaryIdx] = Math.max(minX, Math.min(maxX, normX));
          updateVisorTitles();
          drawVisor(slotNum);
          if (csStatusMsg) {
            const sec = (slot.boundaries[draggingBoundaryIdx] * state.sampleDuration).toFixed(2);
            csStatusMsg.textContent = `✂ Ajustando corte de ${SLICE_NAMES[draggingBoundaryIdx]}: ${sec}s`;
          }
          return;
        }

        hoveredBoundaryIdx = findNearbyBoundary(normX, slot.boundaries);
        canvasEl.style.cursor = hoveredBoundaryIdx > 0 ? 'ew-resize' : 'pointer';
      });

      canvasEl.addEventListener('mousedown', (e) => {
        if (isAutoDemoRunning) stopAutoDemo();
        const slot = slotNum === 1 ? state.slot1 : state.slot2;
        const normX = getNormX(e);

        const bIdx = findNearbyBoundary(normX, slot.boundaries);
        if (bIdx > 0) {
          draggingBoundaryIdx = bIdx;
          setActiveSlot(slotNum);
          e.preventDefault();
          return;
        }

        const sliceIdx = findSliceAtNorm(normX, slot.boundaries);
        playVisorRegion(slotNum, state.mpcEnabled ? sliceIdx : -1, false);
      });

      canvasEl.addEventListener('dblclick', (e) => {
        if (isAutoDemoRunning) stopAutoDemo();
        const slot = slotNum === 1 ? state.slot1 : state.slot2;
        const normX = getNormX(e);
        const sliceIdx = findSliceAtNorm(normX, slot.boundaries);

        // Doble clic: bloquea/desbloquea Corte Exacto en esa rodaja y la reproduce
        if (slot.exactCutLocked && slot.selectedSlice === sliceIdx) {
          slot.exactCutLocked = false;
          playVisorRegion(slotNum, sliceIdx, false);
        } else {
          playVisorRegion(slotNum, sliceIdx, true);
        }
      });

      window.addEventListener('mouseup', () => {
        if (draggingBoundaryIdx > 0) {
          const sliceToPlay = draggingBoundaryIdx;
          draggingBoundaryIdx = -1;
          playVisorRegion(slotNum, sliceToPlay, true);
        }
      });
    }

    attachVisorMouseHandlers(csVisor1Canvas, 1);
    attachVisorMouseHandlers(csVisor2Canvas, 2);

    // Botones del teclado MIDI inferior (#csKbKeys)
    csKbButtons.forEach(btn => {
      btn.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        const sliceIdx = parseInt(btn.getAttribute('data-slice'), 10);
        playVisorRegion(state.activeSlot, sliceIdx, true);
      });
    });

    // Teclado de computadora para disparar las 11 rodajas (Z-E o 1-0) en el visor activo
    const liveKeyToSlice = {
      'z': 0, '1': 0,
      'x': 1, '2': 1,
      'c': 2, '3': 2,
      'v': 3, '4': 3,
      'a': 4, '5': 4,
      's': 5, '6': 5,
      'd': 6, '7': 6,
      'f': 7, '8': 7,
      'q': 8, '9': 8,
      'w': 9, '0': 9,
      'e': 10, '-': 10
    };

    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;
      const k = e.key.toLowerCase();
      if (Object.prototype.hasOwnProperty.call(liveKeyToSlice, k)) {
        if (isAutoDemoRunning) stopAutoDemo();
        const sliceIdx = liveKeyToSlice[k];
        playVisorRegion(state.activeSlot, sliceIdx, true);
      }
    });

    // Botones de selección de ranura Audio 1 / Audio 2
    if (csSlotBtn1) {
      csSlotBtn1.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        const s1 = state.slot1.selectedSlice >= 0 ? state.slot1.selectedSlice : 0;
        playVisorRegion(1, s1, false);
      });
    }
    if (csSlotBtn2) {
      csSlotBtn2.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        const s2 = state.slot2.selectedSlice >= 0 ? state.slot2.selectedSlice : 1;
        playVisorRegion(2, s2, false);
      });
    }

    // Botones de Transformación de la Sección 1
    const csBtnWarmth = document.getElementById('csBtnWarmth');
    const csBtnPitchUp = document.getElementById('csBtnPitchUp');
    const csBtnPitchDown = document.getElementById('csBtnPitchDown');
    const csBtnVerify = document.getElementById('csBtnVerify');
    const csBtnLoadLocal = document.getElementById('csBtnLoadLocal');

    if (csBtnWarmth) {
      csBtnWarmth.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        state.pitchShiftSt = 0;
        if (csRightsBadge) {
          csRightsBadge.style.color = '#00e676';
          csRightsBadge.textContent = '🔥 CALOR ANALÓGICO APLICADO EN VISOR 2';
        }
        playVisorRegion(2, 1, true);
      });
    }

    if (csBtnPitchUp) {
      csBtnPitchUp.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        state.pitchShiftSt = 3;
        const knobText = document.getElementById('csKnobPitchText');
        if (knobText) knobText.textContent = 'Tono: +3 st';
        playVisorRegion(2, state.slot2.selectedSlice >= 0 ? state.slot2.selectedSlice : 2, true);
      });
    }

    if (csBtnPitchDown) {
      csBtnPitchDown.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        state.pitchShiftSt = -3;
        const knobText = document.getElementById('csKnobPitchText');
        if (knobText) knobText.textContent = 'Tono: -3 st';
        playVisorRegion(2, state.slot2.selectedSlice >= 0 ? state.slot2.selectedSlice : 2, true);
      });
    }

    if (csBtnVerify) {
      csBtnVerify.addEventListener('click', () => {
        if (csRightsBadge) {
          csRightsBadge.style.color = '#38bdf8';
          csRightsBadge.textContent = '🔍 Escaneando huella acústica...';
          setTimeout(() => {
            csRightsBadge.style.color = '#00e676';
            csRightsBadge.textContent = '✅ DERECHOS VERIFICADOS: 100% LIBRE DE CONTENT ID';
          }, 450);
        }
        if (csStatusMsg) {
          csStatusMsg.textContent = '🛡️ Análisis completado: El sample transformado en el Visor 2 tiene una huella espectral 100% única y libre de regalías.';
        }
      });
    }

    if (csBtnLoadLocal) {
      csBtnLoadLocal.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        playVisorRegion(1, 0, false);
      });
    }

    const csBtnFreesoundToggle = document.getElementById('csBtnFreesoundToggle');
    if (csBtnFreesoundToggle) {
      csBtnFreesoundToggle.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        playVisorRegion(1, 1, true);
        if (csStatusMsg) {
          csStatusMsg.textContent = '🎵 Sample de Freesound cargado en Visor 1: Dark robotic sample 005 (6.0s, 91.5 BPM, Gm). ¡Tócalo con tu teclado!';
        }
      });
    }

    const csBtnExtract3s = document.getElementById('csBtnExtract3s');
    const csBtnExtract10s = document.getElementById('csBtnExtract10s');
    const csBtnExtract30s = document.getElementById('csBtnExtract30s');
    const csBtnNewExtract = document.getElementById('csBtnNewExtract');

    if (csBtnExtract3s) {
      csBtnExtract3s.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        playVisorRegion(2, 1, true);
        if (csStatusMsg) csStatusMsg.textContent = '⏱ Extracción de 3 Segundos lista en el Visor 2 con Calor Analógico.';
      });
    }

    if (csBtnExtract10s) {
      csBtnExtract10s.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        playVisorRegion(2, 2, true);
        if (csStatusMsg) csStatusMsg.textContent = '⏱ Extracción de 10 Segundos procesada en Visor 2.';
      });
    }

    if (csBtnExtract30s) {
      csBtnExtract30s.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        playVisorRegion(2, -1, false);
        if (csStatusMsg) csStatusMsg.textContent = '⏱ Extracción completa cargada en Visor 2.';
      });
    }

    if (csBtnNewExtract) {
      csBtnNewExtract.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        playVisorRegion(2, 1, false);
        if (csStatusMsg) csStatusMsg.textContent = '✨ Nueva extracción iniciada y lista para rebanar.';
      });
    }

    // Perillas macro interactivas (Lo-Fi, Tono, Filtro)
    const csKnobLofi = document.getElementById('csKnobLofi');
    const csKnobPitch = document.getElementById('csKnobPitch');
    const csKnobFilter = document.getElementById('csKnobFilter');

    if (csKnobLofi) {
      csKnobLofi.addEventListener('click', () => {
        state.lofiIndex = (state.lofiIndex + 1) % 3;
        const labels = ['Lo-Fi: 0%', 'Lo-Fi: 45%', 'Lo-Fi: 85%'];
        const txt = document.getElementById('csKnobLofiText');
        if (txt) txt.textContent = labels[state.lofiIndex];
        const activeSlotObj = state.activeSlot === 1 ? state.slot1 : state.slot2;
        playVisorRegion(state.activeSlot, activeSlotObj.selectedSlice >= 0 ? activeSlotObj.selectedSlice : 1, false);
      });
    }

    if (csKnobPitch) {
      csKnobPitch.addEventListener('click', () => {
        const steps = [0, 3, -3, 5];
        const idx = (steps.indexOf(state.pitchShiftSt) + 1) % steps.length;
        state.pitchShiftSt = steps[idx];
        const sign = state.pitchShiftSt > 0 ? '+' : '';
        const txt = document.getElementById('csKnobPitchText');
        if (txt) txt.textContent = `Tono: ${sign}${state.pitchShiftSt} st`;
        const activeSlotObj = state.activeSlot === 1 ? state.slot1 : state.slot2;
        playVisorRegion(state.activeSlot, activeSlotObj.selectedSlice >= 0 ? activeSlotObj.selectedSlice : 1, false);
      });
    }

    if (csKnobFilter) {
      csKnobFilter.addEventListener('click', () => {
        const filters = [
          { hz: 18000, label: 'Filtro: 100%' },
          { hz: 2400, label: 'Filtro: 55%' },
          { hz: 950, label: 'Filtro: 25%' }
        ];
        const curIdx = filters.findIndex(f => f.hz === state.filterCutoffHz);
        const next = filters[(curIdx + 1) % filters.length];
        state.filterCutoffHz = next.hz;
        const txt = document.getElementById('csKnobFilterText');
        if (txt) txt.textContent = next.label;
        const activeSlotObj = state.activeSlot === 1 ? state.slot1 : state.slot2;
        playVisorRegion(state.activeSlot, activeSlotObj.selectedSlice >= 0 ? activeSlotObj.selectedSlice : 1, false);
      });
    }

    // Botones superiores y de transporte
    const csBtnPlay = document.getElementById('csBtnPlay');
    const csBtnStop = document.getElementById('csBtnStop');
    const csBtnRev = document.getElementById('csBtnRev');
    const csBtnHalf = document.getElementById('csBtnHalf');
    const csBtnExactCut = document.getElementById('csBtnExactCut');
    const csBtnToggleMpc = document.getElementById('csBtnToggleMpc');
    const csBtnSyncBpm = document.getElementById('csBtnSyncBpm');
    const csBtnApplyFx = document.getElementById('csBtnApplyFx');
    const csBtnResetSlices = document.getElementById('csBtnResetSlices');
    const csBtnAutoDemo = document.getElementById('csBtnAutoDemo');
    const csBtnConvertMidi = document.getElementById('csBtnConvertMidi');
    const csBtnDragWav = document.getElementById('csBtnDragWav');
    const csBtnExportWav = document.getElementById('csBtnExportWav');

    if (csBtnPlay) {
      csBtnPlay.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        const slot = state.activeSlot === 1 ? state.slot1 : state.slot2;
        const sliceToPlay = slot.exactCutLocked && slot.selectedSlice >= 0 ? slot.selectedSlice : -1;
        playVisorRegion(state.activeSlot, sliceToPlay, slot.exactCutLocked);
      });
    }

    if (csBtnStop) {
      csBtnStop.addEventListener('click', () => {
        if (isAutoDemoRunning) stopAutoDemo();
        stopCurrentPlayback();
        if (csStatusMsg) csStatusMsg.textContent = '■ Reproducción detenida.';
      });
    }

    if (csBtnRev) {
      csBtnRev.addEventListener('click', () => {
        state.reverse = !state.reverse;
        csBtnRev.classList.toggle('is-on', state.reverse);
        const slot = state.activeSlot === 1 ? state.slot1 : state.slot2;
        playVisorRegion(state.activeSlot, slot.selectedSlice >= 0 ? slot.selectedSlice : 2, false);
      });
    }

    if (csBtnHalf) {
      csBtnHalf.addEventListener('click', () => {
        state.halfSpeed = !state.halfSpeed;
        csBtnHalf.classList.toggle('is-on', state.halfSpeed);
        const slot = state.activeSlot === 1 ? state.slot1 : state.slot2;
        playVisorRegion(state.activeSlot, slot.selectedSlice >= 0 ? slot.selectedSlice : 1, false);
      });
    }

    if (csBtnExactCut) {
      csBtnExactCut.addEventListener('click', () => {
        const slot = state.activeSlot === 1 ? state.slot1 : state.slot2;
        if (slot.selectedSlice < 0) slot.selectedSlice = 1;
        slot.exactCutLocked = !slot.exactCutLocked;
        updateVisorTitles();
        playVisorRegion(state.activeSlot, slot.exactCutLocked ? slot.selectedSlice : -1, slot.exactCutLocked);
      });
    }

    if (csBtnToggleMpc) {
      csBtnToggleMpc.addEventListener('click', () => {
        state.mpcEnabled = !state.mpcEnabled;
        csBtnToggleMpc.textContent = state.mpcEnabled ? 'Rebanador MPC: SÍ' : 'Rebanador MPC: NO';
        csBtnToggleMpc.classList.toggle('cs-btn-mpc-active', state.mpcEnabled);
        redrawBothVisors();
      });
    }

    if (csBtnSyncBpm) {
      csBtnSyncBpm.addEventListener('click', () => {
        state.syncBpm = !state.syncBpm;
        csBtnSyncBpm.textContent = state.syncBpm ? 'Sinc. BPM: SÍ (130)' : 'Sinc. BPM: NO';
        csBtnSyncBpm.classList.toggle('cs-btn-apply-fx', state.syncBpm);
      });
    }

    if (csBtnApplyFx) {
      csBtnApplyFx.addEventListener('click', () => {
        const slot = state.activeSlot === 1 ? state.slot1 : state.slot2;
        playVisorRegion(state.activeSlot, slot.selectedSlice >= 0 ? slot.selectedSlice : 1, true);
      });
    }

    if (csBtnResetSlices) {
      csBtnResetSlices.addEventListener('click', () => {
        state.slot1.boundaries = [...DEFAULT_BOUNDARIES];
        state.slot2.boundaries = [...DEFAULT_BOUNDARIES];
        state.slot1.exactCutLocked = false;
        state.slot2.exactCutLocked = false;
        state.reverse = false;
        state.halfSpeed = false;
        state.pitchShiftSt = 0;
        if (csBtnRev) csBtnRev.classList.remove('is-on');
        if (csBtnHalf) csBtnHalf.classList.remove('is-on');
        updateVisorTitles();
        redrawBothVisors();
        if (csStatusMsg) {
          csStatusMsg.textContent = '🔄 Cortes MPC (C1–A#1) restaurados a sus posiciones automáticas originales.';
        }
      });
    }

    if (csBtnConvertMidi) {
      csBtnConvertMidi.addEventListener('click', () => {
        if (csStatusMsg) {
          csStatusMsg.textContent = '🎹 ¡Conversión Audio a MIDI lista! En el plugin dentro de tu DAW puedes arrastrar la melodía MIDI directo a tu Piano Roll.';
        }
        playVisorRegion(2, 4, true);
      });
    }

    if (csBtnDragWav || csBtnExportWav) {
      const wavHandler = () => {
        if (csStatusMsg) {
          csStatusMsg.textContent = '💾 En tu DAW puedes arrastrar directamente la rodaja seleccionada en formato .WAV 24-bit a cualquier pista.';
        }
      };
      if (csBtnDragWav) csBtnDragWav.addEventListener('click', wavHandler);
      if (csBtnExportWav) csBtnExportWav.addEventListener('click', wavHandler);
    }

    // Demostración Automática Animada (Alterna Visor 1 y Visor 2 tocando rodajas reales)
    const demoSteps = [
      { slot: 1, slice: 1, msg: '🎵 [Demo en Vivo] Escuchando Visor 1 (Audio Original Freesound) — Rodaja C#1' },
      { slot: 1, slice: 3, msg: '🎵 [Demo en Vivo] Escuchando Visor 1 (Audio Original Freesound) — Rodaja D#1' },
      { slot: 1, slice: 5, msg: '🎵 [Demo en Vivo] Escuchando Visor 1 (Audio Original Freesound) — Rodaja F1' },
      { slot: 2, slice: 1, msg: '🔥 [Demo en Vivo] Cambiando al Visor 2 (Calor Analógico +3 st IA) — Rodaja C#1' },
      { slot: 2, slice: 2, msg: '🔥 [Demo en Vivo] Tocando Visor 2 (Calor Analógico) — Rodaja D1' },
      { slot: 2, slice: 4, msg: '🔥 [Demo en Vivo] Tocando Visor 2 (Calor Analógico) — Rodaja E1' },
      { slot: 2, slice: 6, msg: '🔥 [Demo en Vivo] Tocando Visor 2 (Calor Analógico) — Rodaja F#1' },
      { slot: 2, slice: 8, msg: '🔥 [Demo en Vivo] Tocando Visor 2 (Calor Analógico) — Rodaja G#1' },
      { slot: 2, slice: 9, msg: '🔥 [Demo en Vivo] Tocando Visor 2 (Calor Analógico) — Rodaja A1' }
    ];

    function stopAutoDemo() {
      isAutoDemoRunning = false;
      if (autoDemoTimer) {
        clearInterval(autoDemoTimer);
        autoDemoTimer = null;
      }
      if (csBtnAutoDemo) {
        csBtnAutoDemo.textContent = '▶ Demo Automático';
        csBtnAutoDemo.classList.remove('is-running');
      }
    }

    function startAutoDemo() {
      initAudioContext();
      stopAutoDemo();
      isAutoDemoRunning = true;
      if (csBtnAutoDemo) {
        csBtnAutoDemo.textContent = '■ Detener Demo';
        csBtnAutoDemo.classList.add('is-running');
      }

      let idx = 0;
      const runStep = () => {
        if (!isAutoDemoRunning) return;
        const step = demoSteps[idx % demoSteps.length];
        playVisorRegion(step.slot, step.slice, true);
        if (csStatusMsg) csStatusMsg.textContent = step.msg;
        idx++;
        if (idx >= demoSteps.length * 2) {
          stopAutoDemo();
        }
      };

      runStep();
      autoDemoTimer = setInterval(runStep, 520);
    }

    if (csBtnAutoDemo) {
      csBtnAutoDemo.addEventListener('click', () => {
        if (isAutoDemoRunning) {
          stopAutoDemo();
          stopCurrentPlayback();
        } else {
          startAutoDemo();
        }
      });
    }

    // Inicializa el dibujo y decodifica los audios reales
    updateVisorTitles();
    redrawBothVisors();
    decodeEmbeddedRealSamples();
  }
});
