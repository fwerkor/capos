const topBtn = document.getElementById('top');
if (topBtn) {
  window.addEventListener('scroll', () => {
    topBtn.classList.toggle('show', window.scrollY > 420);
  });
  topBtn.addEventListener('click', () => {
    window.scrollTo({ top: 0, behavior: 'smooth' });
  });
}

const menuToggle = document.querySelector('.mobile-menu-toggle');
const mobileNav = document.getElementById('mobileNav');

if (menuToggle && mobileNav) {
  menuToggle.addEventListener('click', () => {
    const willOpen = !mobileNav.classList.contains('open');
    mobileNav.classList.toggle('open', willOpen);
    mobileNav.hidden = !willOpen;
    menuToggle.setAttribute('aria-expanded', String(willOpen));
  });

  mobileNav.querySelectorAll('a').forEach((a) => {
    a.addEventListener('click', () => {
      mobileNav.classList.remove('open');
      mobileNav.hidden = true;
      menuToggle.setAttribute('aria-expanded', 'false');
    });
  });

  window.addEventListener('resize', () => {
    if (window.innerWidth > 920) {
      mobileNav.classList.remove('open');
      mobileNav.hidden = true;
      menuToggle.setAttribute('aria-expanded', 'false');
    }
  });
}
