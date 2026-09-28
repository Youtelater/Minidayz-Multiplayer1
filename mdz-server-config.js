// The public server this build connects to when it is NOT loaded from a
// server - the APK (file://). Browsers that open the server's own address use
// that server and ignore this. Set it BEFORE packing the APK, e.g.
//   window.MDZ_PUBLIC_RELAY_URL = "wss://mdzplus.duckdns.org/ws";
// Empty = no public server: APK players type an address in Advanced.
// NOW: the Cloudflare quick tunnel of the 2026-09-28 public test. It changes
// whenever cloudflared restarts - then set the new one and repack the APK.
// (The planned permanent name was wss://mdzc.duckdns.org/ws.)
window.MDZ_PUBLIC_RELAY_URL = window.MDZ_PUBLIC_RELAY_URL || "wss://speaks-doubt-advertisement-diamonds.trycloudflare.com/ws";
