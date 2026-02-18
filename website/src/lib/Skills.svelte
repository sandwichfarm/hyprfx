<script>
  const skills = [
    {
      name: 'Install',
      desc: 'Step-by-step installation guide for hyprpm and manual builds',
      path: '.skills/install/SKILL.md',
      icon: 'M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4',
    },
    {
      name: 'Create Effect',
      desc: 'Write a new GPU-accelerated window effect from scratch',
      path: '.skills/create-effect/SKILL.md',
      icon: 'M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4',
    },
  ];

  const baseUrl = 'https://raw.githubusercontent.com/sandwichfarm/hyprfx/main/';
</script>

<div class="w-full max-w-4xl mb-16">
  <h2 class="text-xl sm:text-2xl font-mono font-bold text-center mb-6 bg-clip-text text-transparent bg-gradient-to-r from-hfx-red via-hfx-orange to-hfx-yellow">
    Agent Skills
  </h2>
  <p class="text-gray-500 font-mono text-sm text-center mb-8">
    <a href="https://agentskills.io" target="_blank" rel="noopener" class="text-hfx-blue/70 hover:text-hfx-blue transition-colors">agentskills.io</a> compatible skills for AI-assisted workflows
  </p>
  <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
    {#each skills as skill}
      <a
        href="{baseUrl}{skill.path}"
        target="_blank"
        rel="noopener"
        class="bg-hfx-dark/30 backdrop-blur-xl rounded-xl border border-white/10 hover:border-hfx-orange/30 transition-all duration-300 p-6 group block"
      >
        <div class="flex items-start gap-4">
          <svg class="w-6 h-6 text-hfx-orange/60 group-hover:text-hfx-orange transition-colors shrink-0 mt-0.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d={skill.icon}></path>
          </svg>
          <div>
            <h3 class="font-mono font-bold text-gray-200 group-hover:text-white transition-colors mb-1">{skill.name}</h3>
            <p class="text-gray-500 text-sm font-mono">{skill.desc}</p>
            <code class="text-hfx-orange/40 text-xs font-mono mt-2 block">{skill.path}</code>
          </div>
        </div>
      </a>
    {/each}
  </div>
</div>
