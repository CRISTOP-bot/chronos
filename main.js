/* ── CHRONOS website — main.js ── */

// ── Nav scroll shadow ─────────────────────────────────────────────────────
const nav = document.getElementById('nav');
window.addEventListener('scroll', () => {
  nav.classList.toggle('scrolled', window.scrollY > 20);
}, { passive: true });

// ── Mobile burger ─────────────────────────────────────────────────────────
const burger  = document.getElementById('navBurger');
const navLinks = document.querySelector('.nav-links');
burger.addEventListener('click', () => {
  const open = burger.classList.toggle('open');
  navLinks.classList.toggle('open', open);
  burger.setAttribute('aria-expanded', String(open));
});
document.addEventListener('click', (e) => {
  if (!nav.contains(e.target)) {
    burger.classList.remove('open');
    navLinks.classList.remove('open');
    burger.setAttribute('aria-expanded', 'false');
  }
});

// ── Scroll-to-reveal (data-aos) ───────────────────────────────────────────
const aosEls = document.querySelectorAll('[data-aos]');
const aosObs = new IntersectionObserver((entries) => {
  entries.forEach(entry => {
    if (entry.isIntersecting) {
      entry.target.classList.add('aos-visible');
      aosObs.unobserve(entry.target);
    }
  });
}, { threshold: 0.1 });
aosEls.forEach(el => aosObs.observe(el));

// ── Hero canvas — animated event timeline ─────────────────────────────────
(function () {
  const canvas = document.getElementById('heroCanvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');

  const TICK_TYPES = [
    { color: '#4caf50', h: 14, w: 2 },
    { color: '#ffb300', h: 22, w: 3 },
    { color: '#64b5f6', h: 10, w: 2 },
    { color: '#ce93d8', h: 8,  w: 1 },
    { color: '#ef9a9a', h: 6,  w: 1 },
  ];
  const NUM_LINES = 6;
  let W, H, lines = [];

  function init() {
    W = canvas.width  = canvas.offsetWidth;
    H = canvas.height = canvas.offsetHeight;
    lines = [];
    for (let l = 0; l < NUM_LINES; l++) {
      const ticks = [];
      const count = Math.floor(W / 12) + 12;
      for (let i = 0; i < count; i++) {
        ticks.push({
          x: Math.random() * W * 1.5 - W * 0.25,
          type: TICK_TYPES[Math.floor(Math.random() * TICK_TYPES.length)],
          alpha: Math.random() * 0.55 + 0.1,
        });
      }
      lines.push({
        y: (l + 0.5) / NUM_LINES * H,
        ticks,
        speed: (Math.random() * 0.25 + 0.1) * (Math.random() < 0.5 ? 1 : -1),
        offset: Math.random() * W,
      });
    }
  }

  function draw() {
    ctx.clearRect(0, 0, W, H);
    lines.forEach(line => {
      line.offset = (line.offset + line.speed + W * 1.5) % (W * 1.5);
      ctx.beginPath();
      ctx.strokeStyle = 'rgba(42,46,56,0.5)';
      ctx.lineWidth = 1;
      ctx.moveTo(0, line.y); ctx.lineTo(W, line.y);
      ctx.stroke();
      line.ticks.forEach(tick => {
        const x = (tick.x + line.offset) % (W * 1.5) - W * 0.25;
        if (x < -10 || x > W + 10) return;
        ctx.beginPath();
        ctx.strokeStyle = tick.type.color;
        ctx.globalAlpha = tick.alpha;
        ctx.lineWidth = tick.type.w;
        ctx.lineCap = 'round';
        ctx.moveTo(x, line.y - tick.type.h / 2);
        ctx.lineTo(x, line.y + tick.type.h / 2);
        ctx.stroke();
        ctx.globalAlpha = 1;
      });
    });

    // scanning needle
    const prog = (Date.now() % 5000) / 5000;
    const nx = prog * W;
    const g = ctx.createLinearGradient(nx - 80, 0, nx + 80, 0);
    g.addColorStop(0,   'rgba(70,130,180,0)');
    g.addColorStop(0.5, 'rgba(100,181,246,0.2)');
    g.addColorStop(1,   'rgba(70,130,180,0)');
    ctx.fillStyle = g;
    ctx.fillRect(nx - 80, 0, 160, H);
    ctx.beginPath();
    ctx.strokeStyle = 'rgba(100,181,246,0.35)';
    ctx.lineWidth = 1;
    ctx.moveTo(nx, 0); ctx.lineTo(nx, H);
    ctx.stroke();

    requestAnimationFrame(draw);
  }

  new ResizeObserver(init).observe(canvas.parentElement);
  init();
  draw();
})();

// ── Terminal typewriter animation ─────────────────────────────────────────
(function () {
  const body = document.getElementById('terminalBody');
  if (!body) return;

  const LINES = [
    { type: 'prompt', text: 'chronos_cli trace /bin/ls recording.chronos' },
    { type: 'out',    text: 'CHRONOS 0.1.0 - Temporal Execution & Reconstruction Engine' },
    { type: 'hi',     text: 'Traced 88 events → recording.chronos' },
    { type: 'blank' },
    { type: 'prompt', text: 'chronos_cli info recording.chronos' },
    { type: 'out',    text: 'name=trace  events=88  checkpoints=5  target=/bin/ls' },
    { type: 'dim',    text: '  SyscallEnter: 41   SyscallExit: 40' },
    { type: 'dim',    text: '  Checkpoint:  5     ProcessExit: 1' },
    { type: 'blank' },
    { type: 'prompt', text: 'chronos_cli memdiff recording.chronos 1 5' },
    { type: 'out',    text: 'Comparing checkpoint 1 vs 5' },
    { type: 'warn',   text: '  changed 0x5606fcc1c000: 364 bytes differ' },
    { type: 'warn',   text: '  added   0x7fc024387000 (8192 bytes)' },
    { type: 'blank' },
    { type: 'prompt', text: 'chronos_cli stats recording.chronos' },
    { type: 'out',    text: 'Total events: 88  |  Unique syscalls: 18' },
    { type: 'dim',    text: '  write: 1   openat: 3   mmap: 9   brk: 3' },
  ];

  const CHAR_DELAY   = 28;   // ms per character for prompt lines
  const LINE_PAUSE   = 320;  // ms after a non-prompt line appears
  const PROMPT_PAUSE = 180;  // ms before prompt starts typing
  const LOOP_PAUSE   = 2400; // ms before restarting

  let currentLine = 0;
  let currentChar = 0;
  let el = null;
  let timer = null;

  function esc(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
  }

  function lineClass(type) {
    switch (type) {
      case 'out':    return 't-line t-out';
      case 'hi':     return 't-line t-out';
      case 'dim':    return 't-line t-out t-dim';
      case 'warn':   return 't-line t-out t-warn';
      case 'blank':  return 't-line t-blank';
      default:       return 't-line';
    }
  }

  function removeCursor() {
    const cur = body.querySelector('.t-cursor');
    if (cur) cur.remove();
  }

  function addCursor(parent) {
    const c = document.createElement('span');
    c.className = 't-cursor';
    c.textContent = '▋';
    parent.appendChild(c);
  }

  function appendLine(type, text) {
    const div = document.createElement('div');
    div.className = lineClass(type);
    if (type === 'blank') { body.appendChild(div); return div; }
    if (type === 'prompt') {
      const prompt = document.createElement('span');
      prompt.className = 't-prompt';
      prompt.textContent = '$ ';
      div.appendChild(prompt);
      const cmd = document.createElement('span');
      cmd.className = 't-cmd';
      div.appendChild(cmd);
    } else {
      // colorize highlights in non-prompt lines
      div.innerHTML = esc(text)
        .replace(/(\d+) events/g, '<span class="t-hl">$1 events</span>')
        .replace(/checkpoints=(\d+)/g, 'checkpoints=<span class="t-hl">$1</span>')
        .replace(/events=(\d+)/g, 'events=<span class="t-hl">$1</span>')
        .replace(/(changed|added)/g, '<span class="t-warn">$1</span>');
    }
    body.appendChild(div);
    return div;
  }

  function step() {
    if (currentLine >= LINES.length) {
      // end of script — pause then restart
      removeCursor();
      timer = setTimeout(restart, LOOP_PAUSE);
      return;
    }

    const line = LINES[currentLine];

    if (line.type === 'blank') {
      appendLine('blank');
      currentLine++;
      timer = setTimeout(step, 80);
      return;
    }

    if (line.type === 'prompt') {
      if (currentChar === 0) {
        removeCursor();
        el = appendLine('prompt', line.text);
        timer = setTimeout(step, PROMPT_PAUSE);
        currentChar = 1;
        return;
      }
      // typing
      const cmdSpan = el.querySelector('.t-cmd');
      if (currentChar <= line.text.length) {
        removeCursor();
        cmdSpan.textContent = line.text.slice(0, currentChar);
        addCursor(cmdSpan);
        currentChar++;
        timer = setTimeout(step, CHAR_DELAY);
      } else {
        // done typing — press enter
        removeCursor();
        currentLine++;
        currentChar = 0;
        timer = setTimeout(step, 120);
      }
      return;
    }

    // instant output line
    removeCursor();
    appendLine(line.type, line.text);
    currentLine++;
    currentChar = 0;
    // scroll terminal
    body.scrollTop = body.scrollHeight;
    timer = setTimeout(step, LINE_PAUSE);
  }

  function restart() {
    clearTimeout(timer);
    body.innerHTML = '';
    currentLine = 0;
    currentChar = 0;
    el = null;
    timer = setTimeout(step, 600);
  }

  // Start after a short delay so the page can render first
  timer = setTimeout(step, 800);
})();

// ── Active nav section highlight ──────────────────────────────────────────
const sections   = document.querySelectorAll('section[id]');
const navAnchors = document.querySelectorAll('.nav-links a[href^="#"]');
new IntersectionObserver((entries) => {
  entries.forEach(entry => {
    if (entry.isIntersecting) {
      navAnchors.forEach(a => {
        const active = a.getAttribute('href') === '#' + entry.target.id;
        a.style.color    = active ? 'var(--text)'   : '';
        a.style.background = active ? 'var(--bg4)' : '';
      });
    }
  });
}, { rootMargin: '-35% 0px -60% 0px' }).observe;
sections.forEach(s => {
  new IntersectionObserver((entries) => {
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        navAnchors.forEach(a => {
          const active = a.getAttribute('href') === '#' + entry.target.id;
          a.style.color      = active ? 'var(--text)' : '';
          a.style.background = active ? 'var(--bg4)' : '';
        });
      }
    });
  }, { rootMargin: '-35% 0px -55% 0px' }).observe(s);
});

// ── Copy-to-clipboard on code blocks ─────────────────────────────────────
document.querySelectorAll('pre').forEach(pre => {
  const btn = document.createElement('button');
  btn.className  = 'copy-btn';
  btn.textContent = 'copy';
  btn.setAttribute('aria-label', 'Copy code');
  pre.style.position = 'relative';
  pre.appendChild(btn);
  btn.addEventListener('click', async () => {
    const text = pre.textContent.replace(/copy\s*$/, '').trim();
    try {
      await navigator.clipboard.writeText(text);
      btn.textContent = '✓ copied';
      setTimeout(() => { btn.textContent = 'copy'; }, 1800);
    } catch { btn.textContent = 'error'; }
  });
});
