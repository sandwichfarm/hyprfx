<script>
  const effects = [
    {
      name: 'Fire',
      key: 'fire',
      description: 'Windows burn away with a procedural flame simulation using 3D simplex noise',
      color: 'from-hfx-red to-hfx-orange',
      borderHover: 'hover:border-hfx-orange/50',
      preview: null, // placeholder for gif
    },
    {
      name: 'TV',
      key: 'tv',
      description: 'Classic CRT television power-off effect with horizontal and vertical collapse',
      color: 'from-blue-400 to-hfx-blue',
      borderHover: 'hover:border-hfx-blue/50',
      preview: null,
    },
    {
      name: 'Pixelate',
      key: 'pixelate',
      description: 'Windows dissolve into increasingly large pixels with noise-driven fade',
      color: 'from-green-400 to-emerald-400',
      borderHover: 'hover:border-green-400/50',
      preview: null,
    },
  ];
</script>

<div class="w-full max-w-4xl mb-16">
  <h2 class="text-xl sm:text-2xl font-mono font-bold text-center mb-10 bg-clip-text text-transparent bg-gradient-to-r from-hfx-red via-hfx-orange to-hfx-yellow">
    Effects
  </h2>
  <div class="grid grid-cols-1 md:grid-cols-3 gap-6">
    {#each effects as effect}
      <div class="bg-hfx-dark/30 backdrop-blur-xl rounded-xl border border-white/10 {effect.borderHover} transition-all duration-300 overflow-hidden group">
        <div class="aspect-video bg-gradient-to-br from-hfx-dark to-hfx-darker flex items-center justify-center border-b border-white/5">
          {#if effect.preview}
            <img src={effect.preview} alt="{effect.name} effect demo" class="w-full h-full object-cover" loading="lazy" />
          {:else}
            <div class="text-center">
              <p class="text-gray-600 font-mono text-xs">gif placeholder</p>
            </div>
          {/if}
        </div>
        <div class="p-5">
          <h3 class="font-mono font-bold mb-1">
            <span class="bg-clip-text text-transparent bg-gradient-to-r {effect.color}">{effect.name}</span>
          </h3>
          <code class="text-gray-500 text-xs font-mono block mb-2">plugin:hfx:close_effect = {effect.key}</code>
          <p class="text-gray-400 text-sm font-mono">{effect.description}</p>
        </div>
      </div>
    {/each}
  </div>
</div>
