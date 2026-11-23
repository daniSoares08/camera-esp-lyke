-- Dedicated private archive. No browser/client access to rows or image objects.
create table public.lyke_requests (
  id text primary key check (id ~ '^[A-Za-z0-9_-]{16,64}$'),
  device_id text not null check (length(device_id) between 1 and 64),
  input_fingerprint text not null check (input_fingerprint ~ '^[a-f0-9]{64}$'),
  created_at timestamptz not null default now(),
  finished_at timestamptz,
  status text not null default 'processing' check (status in ('processing','completed','failed')),
  model text not null,
  photo_count smallint not null check (photo_count between 1 and 3),
  photo_paths jsonb not null default '[]'::jsonb,
  answer_text text,
  answer_lcd text check (length(answer_lcd) <= 12000),
  input_tokens integer not null default 0 check (input_tokens >= 0),
  output_tokens integer not null default 0 check (output_tokens >= 0),
  openai_response_id text,
  error jsonb
);
create index lyke_requests_device_created_idx on public.lyke_requests (device_id, created_at desc);
alter table public.lyke_requests enable row level security;
revoke all on public.lyke_requests from public, anon, authenticated;
grant select, insert, update on public.lyke_requests to service_role;

create function public.lyke_claim_request(
  p_id text, p_device_id text, p_fingerprint text, p_model text,
  p_photo_count integer, p_daily_limit integer default 50
) returns jsonb language plpgsql security invoker set search_path = '' as $$
declare existing public.lyke_requests%rowtype;
begin
  -- Serialize reservations per device so concurrent requests cannot exceed the cap.
  perform pg_catalog.pg_advisory_xact_lock(pg_catalog.hashtext(p_device_id)::bigint);
  select * into existing from public.lyke_requests where id = p_id;
  if found then
    if existing.device_id <> p_device_id or existing.input_fingerprint <> p_fingerprint then
      return jsonb_build_object('decision','mismatch');
    end if;
    return jsonb_build_object('decision',case existing.status
      when 'completed' then 'cached' when 'failed' then 'failed' else 'busy' end,
      'request',to_jsonb(existing));
  end if;
  if (select count(*) from public.lyke_requests
      where device_id = p_device_id and created_at >= now() - interval '24 hours')
      >= greatest(1,least(100,p_daily_limit)) then
    return jsonb_build_object('decision','limit');
  end if;
  insert into public.lyke_requests (id,device_id,input_fingerprint,model,photo_count)
    values (p_id,p_device_id,p_fingerprint,p_model,p_photo_count);
  return jsonb_build_object('decision','start');
end;
$$;
revoke all on function public.lyke_claim_request(text,text,text,text,integer,integer) from public, anon, authenticated;
grant execute on function public.lyke_claim_request(text,text,text,text,integer,integer) to service_role;

insert into storage.buckets (id, name, public, file_size_limit, allowed_mime_types)
values ('lyke-images','lyke-images',false,524288,array['image/jpeg']);
-- No anon/authenticated policies: images can only be read by the backend/admin.
