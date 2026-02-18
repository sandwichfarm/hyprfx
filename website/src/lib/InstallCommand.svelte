<script>
  let copied = $state(false);
  const command = 'hyprpm add https://github.com/sandwichfarm/hyprfx && hyprpm enable hyprfx';

  const copy = async () => {
    await navigator.clipboard.writeText(command);
    copied = true;
    setTimeout(() => copied = false, 2000);
  };
</script>

<div class="w-full max-w-2xl mb-16">
  <h2 class="text-xl sm:text-2xl font-mono font-bold text-center mb-6 bg-clip-text text-transparent bg-gradient-to-r from-hfx-red via-hfx-orange to-hfx-yellow">
    Install
  </h2>
  <button
    onclick={copy}
    class="w-full group relative bg-hfx-dark/50 backdrop-blur-xl rounded-xl border border-white/10 hover:border-hfx-orange/30 transition-all duration-300 p-4 text-left cursor-pointer"
  >
    <div class="flex items-center gap-3">
      <span class="text-hfx-orange/60 font-mono text-sm select-none shrink-0">$</span>
      <code class="text-gray-300 font-mono text-xs sm:text-sm break-all">{command}</code>
      <span class="shrink-0 ml-auto text-gray-500 group-hover:text-hfx-orange transition-colors">
        {#if copied}
          <svg class="w-5 h-5 text-green-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M5 13l4 4L19 7"></path>
          </svg>
        {:else}
          <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M8 16H6a2 2 0 01-2-2V6a2 2 0 012-2h8a2 2 0 012 2v2m-6 12h8a2 2 0 002-2v-8a2 2 0 00-2-2h-8a2 2 0 00-2 2v8a2 2 0 002 2z"></path>
          </svg>
        {/if}
      </span>
    </div>
  </button>
  <p class="text-gray-600 text-xs font-mono text-center mt-3">click to copy</p>
</div>
