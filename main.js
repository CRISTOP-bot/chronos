/* ── CHRONOS website — main.js ── */

// ── Nav scroll shadow ─────────────────────────────────────────────────────
const nav = document.getElementById('nav');
window.addEventListener('scroll', () => {
  nav.classList.toggle('scrolled', window.scrollY > 20);
}, { passive: true });

// ── Mobile burger ─────────────────────────────────────────────────────────
const burger = document.getElementById('navBurger');
const navLinks = document.querySelector('.nav-links');
burger.addEventListener('click', () => {
  const open = burger.classList.toggle('open');
  navLinks.classList.toggle('open', open);
  burger.setAttribute('aria-expanded', String(open));
});
// Close on outside click
document.addEventListener('click', (e) => {
  if (!nav.contains(e.target)) {
    burger.classList.remove('open');
    navLinks.classList.remove('open');
    burger.setAttribute('aria-expanded', 'false');
  }
});

// ── Scroll-to-reveal (data-aos) ───────────────────────────────────────────
const aosEls = document.querySelectorAll('[data-aos]');
const observer = new IntersectionObserver((entries) => {
  entries.forEach(entry => {
    if (entry.isIntersecting) {
      entry.target.classList.add('aos-visible');
      observer.unobserve(entry.target);
    }
  });
}, { threshold: 0.12 });
aosEls.forEach(el => observer.observe(el));

// ── Hero canvas — animated event timeline particles ───────────────────────
(function () {
  const canvas = document.getElementById('heroCanvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');

  let W, H;
  const COLORS = ['#4caf50', '#ffb300', '#64b5f6', '#ce93d8', '#ef9a9a', '#4682b4'];
  const TICK_TYPES = [
    { color: '#4caf50', h: 14, w: 2  },  // syscall
    { color: '#ffb300', h: 22, w: 3  },  // checkpoint
    { color: '#64b5f6', h: 10, w: 2  },  // function
    { color: '#ce93d8', h: 8,  w: 1  },  // variable
    { color: '#ef9a9a', h: 6,  w: 1  },  // memory
  ];

  // Lines of ticks (timelines)
  const NUM_LINES = 5;
  const lines = [];

  function init() {
    W = canvas.width  = canvas.offsetWidth;
    H = canvas.height = canvas.offsetHeight;
    lines.length = 0;
    for (let l = 0; l < NUM_LINES; l++) {
      const ticks = [];
      const count = Math.floor(W / 14) + 10;
      for (let i = 0; i < count; i++) {
        const type = TICK_TYPES[Math.floor(Math.random() * TICK_TYPES.length)];
        ticks.push({
          x: Math.random() * W * 1.4 - W * 0.2,
          type,
          alpha: Math.random() * 0.5 + 0.15,
        });
      }
      lines.push({
        y: (l + 1) / (NUM_LINES + 1) * H,
        ticks,
        speed: (Math.random() * 0.3 + 0.15) * (Math.random() < 0.5 ? 1 : -1),
        offset: 0,
      });
    }
  }

  let raf;
  function draw() {
    ctx.clearRect(0, 0, W, H);

    lines.forEach(line => {
      line.offset = (line.offset + line.speed) % W;
      const lx = line.offset;

      // baseline
      ctx.beginPath();
      ctx.strokeStyle = 'rgba(42,46,56,0.6)';
      ctx.lineWidth = 1;
      ctx.moveTo(0, line.y);
      ctx.lineTo(W, line.y);
      ctx.stroke();

      // ticks
      line.ticks.forEach(tick => {
        let x = (tick.x + lx) % (W * 1.4) - W * 0.2;
        if (x < -20 || x > W + 20) return;
        const { color, h, w } = tick.type;
        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.globalAlpha = tick.alpha;
        ctx.lineWidth = w;
        ctx.lineCap = 'round';
        ctx.moveTo(x, line.y - h / 2);
        ctx.lineTo(x, line.y + h / 2);
        ctx.stroke();
        ctx.globalAlpha = 1;
      });
    });

    // needle — a vertical scanning line
    const progress = (Date.now() % 6000) / 6000;
    const nx = progress * W;
    const grad = ctx.createLinearGradient(nx - 60, 0, nx + 60, 0);
    grad.addColorStop(0,   'rgba(70,130,180,0)');
    grad.addColorStop(0.5, 'rgba(100,181,246,0.25)');
    grad.addColorStop(1,   'rgba(70,130,180,0)');
    ctx.fillStyle = grad;
    ctx.fillRect(nx - 60, 0, 120, H);

    ctx.beginPath();
    ctx.strokeStyle = 'rgba(100,181,246,0.4)';
    ctx.lineWidth = 1;
    ctx.moveTo(nx, 0);
    ctx.lineTo(nx, H);
    ctx.stroke();

    raf = requestAnimationFrame(draw);
  }

  const ro = new ResizeObserver(() => { init(); });
  ro.observe(canvas.parentElement);
  init();
  draw();
})();

// ── Smooth active nav link highlighting ──────────────────────────────────
const sections = document.querySelectorAll('section[id]');
const navAnchors = document.querySelectorAll('.nav-links a[href^="#"]');
const ioNav = new IntersectionObserver((entries) => {
  entries.forEach(entry => {
    if (entry.isIntersecting) {
      navAnchors.forEach(a => {
        a.style.color = a.getAttribute('href') === '#' + entry.target.id
          ? 'var(--text)' : '';
      });
    }
  });
}, { rootMargin: '-40% 0px -55% 0px' });
sections.forEach(s => ioNav.observe(s));
