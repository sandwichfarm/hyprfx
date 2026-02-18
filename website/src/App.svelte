<script>
  import { onMount } from 'svelte';
  import Hero from './lib/Hero.svelte';
  import Effects from './lib/Effects.svelte';
  import InstallCommand from './lib/InstallCommand.svelte';
  import Config from './lib/Config.svelte';
  import Skills from './lib/Skills.svelte';
  import SocialLinks from './lib/SocialLinks.svelte';
  import Footer from './lib/Footer.svelte';

  let mouseX = 0.5;
  let mouseY = 0.5;
  let targetX = 0.5;
  let targetY = 0.5;
  let animationFrame;

  const handleMouseMove = (e) => {
    targetX = e.clientX / window.innerWidth;
    targetY = e.clientY / window.innerHeight;
  };

  const animate = () => {
    const easing = 0.08;
    const time = Date.now() * 0.0001;
    const idleX = Math.sin(time) * 0.02;
    const idleY = Math.cos(time * 0.7) * 0.02;
    mouseX += (targetX + idleX - mouseX) * easing;
    mouseY += (targetY + idleY - mouseY) * easing;
    animationFrame = requestAnimationFrame(animate);
  };

  onMount(() => {
    window.addEventListener('mousemove', handleMouseMove);
    animate();
    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      if (animationFrame) cancelAnimationFrame(animationFrame);
    };
  });
</script>

<main class="min-h-screen w-full flex flex-col items-center relative mb-4">
  <div
    class="fixed inset-0 -z-10"
    style="background: linear-gradient(135deg,
           #0A0E1B,
           #0D0808 {45 + (mouseX - 0.5) * 10}%,
           #1A0A00 {55 + (mouseY - 0.5) * 10}%,
           #050810);
           background-size: 130% 130%;
           background-position: {50 - (mouseX - 0.5) * 15}% {50 - (mouseY - 0.5) * 15}%"
  ></div>
  <div class="fixed inset-0 opacity-[0.06] -z-10" style="background-image: url('data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIzMDAiIGhlaWdodD0iMzAwIj48ZmlsdGVyIGlkPSJuIj48ZmVUdXJidWxlbmNlIHR5cGU9ImZyYWN0YWxOb2lzZSIgYmFzZUZyZXF1ZW5jeT0iLjciIG51bU9jdGF2ZXM9IjQiIHN0aXRjaFRpbGVzPSJzdGl0Y2giLz48L2ZpbHRlcj48cmVjdCB3aWR0aD0iMTAwJSIgaGVpZ2h0PSIxMDAlIiBmaWx0ZXI9InVybCgjbikiIG9wYWNpdHk9IjEiLz48L3N2Zz4=');"></div>

  <div class="relative z-10 w-full max-w-6xl mx-auto px-4 sm:px-6 lg:px-8 flex flex-col items-center">
    <Hero {mouseX} {mouseY} />
    <Effects />
    <InstallCommand />
    <Config />
    <Skills />
    <SocialLinks />
    <Footer />
  </div>
</main>

<style>
  :global(html) {
    scroll-behavior: smooth;
  }
</style>
