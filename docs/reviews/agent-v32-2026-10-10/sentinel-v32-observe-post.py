import pty,fcntl,termios,struct,subprocess,os,time,select,socket,json
root='/var/folders/jn/rtnh3wpd52537ppg_tls1nm40000gn/T/sentinel-v32-isolated-m0n8zzab';path=root+'/run/daemon.sock';s=socket.socket(socket.AF_UNIX);s.settimeout(3);s.connect(path);f=s.makefile('rb')
for i,(name,payload) in enumerate([('hello',{'client_id':'v32-ui-observer','major':1,'minor':1,'capabilities':[]}),('session.list',{})]):
 s.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'request','id':str(i),'name':name,'payload':payload})+'\n').encode())
 while True:
  e=json.loads(f.readline())
  if e.get('id')==str(i):break
sid=next(x['session_id'] for x in e['payload']['sessions'] if x['title'].startswith('V32 level2'))
for width,height in [(80,24),(120,40)]:
 m,sl=pty.openpty();fcntl.ioctl(sl,termios.TIOCSWINSZ,struct.pack('HHHH',height,width,0,0));env=dict(os.environ,TERM='xterm-256color');env.pop('NO_COLOR',None);p=subprocess.Popen(['cli/target/release/sentinel','--socket',path,'tui','--session',sid],stdin=sl,stdout=sl,stderr=sl,env=env,start_new_session=True);os.close(sl);data=b''
 for stage,keys in [('attached',b''),('details',b'/details\r')]:
  if keys:os.write(m,keys)
  end=time.monotonic()+1
  while time.monotonic()<end:
   if select.select([m],[],[],.02)[0]:
    try:data+=os.read(m,65536)
    except OSError:break
  open('docs/reviews/post-fix-agent-v32-2026-10-10/post-fix-agent-'+stage+'-'+str(width)+'x'+str(height)+'.ansi','wb').write(data)
 p.terminate();p.wait(5);os.close(m)
s.close()
