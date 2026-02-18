<script>
  const configs = [
    { key: 'plugin:hfx:open_effect', type: 'string', default: 'none', desc: 'Effect for opening windows' },
    { key: 'plugin:hfx:close_effect', type: 'string', default: 'fire', desc: 'Effect for closing windows' },
    { key: 'plugin:hfx:open_duration', type: 'float', default: '1.0', desc: 'Opening animation duration (seconds)' },
    { key: 'plugin:hfx:close_duration', type: 'float', default: '1.0', desc: 'Closing animation duration (seconds)' },
    { key: 'plugin:hfx:fire_color_1', type: 'color', default: 'rgba(FF4400FF)', desc: 'Fire inner color' },
    { key: 'plugin:hfx:fire_color_2', type: 'color', default: 'rgba(FF8800FF)', desc: 'Fire mid color' },
    { key: 'plugin:hfx:fire_color_3', type: 'color', default: 'rgba(FFCC00FF)', desc: 'Fire outer color' },
    { key: 'plugin:hfx:fire_scale', type: 'float', default: '4.0', desc: 'Fire noise scale' },
    { key: 'plugin:hfx:fire_speed', type: 'float', default: '1.0', desc: 'Fire animation speed' },
    { key: 'plugin:hfx:tv_color', type: 'color', default: 'rgba(00D9FFFF)', desc: 'TV glow color' },
  ];
</script>

<div class="w-full max-w-4xl mb-16">
  <h2 class="text-xl sm:text-2xl font-mono font-bold text-center mb-6 bg-clip-text text-transparent bg-gradient-to-r from-hfx-red via-hfx-orange to-hfx-yellow">
    Configuration
  </h2>
  <div class="bg-hfx-dark/30 backdrop-blur-xl rounded-xl border border-white/10 overflow-hidden">
    <div class="overflow-x-auto">
      <table class="w-full text-sm font-mono">
        <thead>
          <tr class="border-b border-white/10 text-gray-400">
            <th class="text-left p-4 font-normal">Key</th>
            <th class="text-left p-4 font-normal hidden sm:table-cell">Type</th>
            <th class="text-left p-4 font-normal">Default</th>
            <th class="text-left p-4 font-normal hidden md:table-cell">Description</th>
          </tr>
        </thead>
        <tbody>
          {#each configs as cfg, i}
            <tr class="border-b border-white/5 {i % 2 === 0 ? 'bg-white/[0.02]' : ''}">
              <td class="p-4 text-hfx-orange text-xs">{cfg.key}</td>
              <td class="p-4 text-gray-500 text-xs hidden sm:table-cell">{cfg.type}</td>
              <td class="p-4 text-gray-300 text-xs">{cfg.default}</td>
              <td class="p-4 text-gray-400 text-xs hidden md:table-cell">{cfg.desc}</td>
            </tr>
          {/each}
        </tbody>
      </table>
    </div>
  </div>
</div>
