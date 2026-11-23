-- The Dashboard's automatic-RLS helper is privileged and is not a client RPC.
do $$
begin
  if to_regprocedure('public.rls_auto_enable()') is not null then
    revoke execute on function public.rls_auto_enable() from public, anon, authenticated;
  end if;
end;
$$;

-- Intentional backend-only access; no policy grants any end-user role access.
create policy backend_service_only on public.lyke_requests
  for all to service_role using (true) with check (true);
