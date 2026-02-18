<script>
  export let mouseX = 0.5;
  export let mouseY = 0.5;
</script>

<div class="mt-24 mb-8 text-center">
  <h1 class="text-5xl sm:text-6xl md:text-7xl lg:text-8xl font-mono font-bold mb-4">
    <span class="bg-clip-text text-transparent bg-gradient-to-r from-hfx-red via-hfx-orange to-hfx-yellow">
      hyprfx
    </span>
  </h1>
  <p class="text-gray-400 text-lg sm:text-xl md:text-2xl font-mono">
    gpu-accelerated window effects for hyprland
  </p>
</div>

<div class="w-full max-w-4xl aspect-video mb-12 relative group">
  <div
    class="absolute inset-0 bg-gradient-to-r from-hfx-red/20 via-hfx-orange/20 to-hfx-yellow/20 rounded-2xl blur-3xl group-hover:blur-2xl transition-all duration-500 animate-glow"
    style="transform: translate({(mouseX - 0.5) * 8}px, {(mouseY - 0.5) * 8}px)"
  ></div>
  <div class="relative bg-hfx-dark/80 backdrop-blur-xl rounded-2xl border border-white/10 overflow-hidden shadow-2xl flex items-center justify-center">
    <!-- Replace with your demo video/gif -->
    <div class="w-full aspect-video flex items-center justify-center bg-gradient-to-br from-hfx-dark to-hfx-darker">
      <div class="text-center">
        <svg class="w-16 h-16 mx-auto mb-4 text-hfx-orange/40" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z"></path>
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z"></path>
        </svg>
        <p class="text-gray-500 font-mono text-sm">demo video placeholder</p>
      </div>
    </div>
  </div>
</div>
