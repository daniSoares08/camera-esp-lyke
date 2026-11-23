import { createHandler } from './handler.ts';

const secretKeys = JSON.parse(Deno.env.get('SUPABASE_SECRET_KEYS') || '{}');
Deno.serve(createHandler({
  supabaseUrl: Deno.env.get('SUPABASE_URL') || '',
  serviceKey: secretKeys.default || Deno.env.get('SUPABASE_SERVICE_ROLE_KEY') || '',
  openaiKey: Deno.env.get('OPENAI_API_KEY') || '',
  tokenHash: Deno.env.get('LYKE_DEVICE_TOKEN_SHA256') || '',
  deviceId: Deno.env.get('LYKE_DEVICE_ID') || 'lyke-01',
  dailyLimit: Math.max(1, Math.min(100, Number(Deno.env.get('LYKE_DAILY_LIMIT') || 50))),
}));
