#!/usr/bin/python
#coding=utf8
"""
import sys
reload(sys)
sys.setdefaultencoding('utf-8')
"""
from BaseHTTPServer import BaseHTTPRequestHandler,HTTPServer
from os import curdir, sep
import cgi
import logging
import time

PORT_NUMBER = 8090
RES_FILE_DIR = "."

class myHandler(BaseHTTPRequestHandler):
	
	def do_GET(self):
		if self.path=="/":
			self.path="/index_example3.html"

		try:
			#根据请求的文件扩展名，设置正确的mime类型
			sendReply = False
			if self.path.endswith(".html"):
				mimetype='text/html'
				sendReply = True
			if self.path.endswith(".jpg"):
				mimetype='image/jpg'
				sendReply = True
			if self.path.endswith(".gif"):
				mimetype='image/gif'
				sendReply = True
			if self.path.endswith(".js"):
				mimetype='application/javascript'
				sendReply = True
			if self.path.endswith(".css"):
				mimetype='text/css'
				sendReply = True

			if sendReply == True:
				#读取相应的静态资源文件，并发送它
				f = open(curdir + sep + self.path, 'rb')
				self.send_response(200)
				self.send_header('Content-type',mimetype)
				self.end_headers()
				self.wfile.write(f.read())
				f.close()
			return

		except IOError:
			self.send_error(404,'File Not Found: %s' % self.path)

	def do_POST(self):
                print "---------do_post-------"
		logging.warning(self.headers)
                '''
		form = cgi.FieldStorage(
			fp=self.rfile,
			headers=self.headers,
			environ={'REQUEST_METHOD':'POST',
					'CONTENT_TYPE':self.headers['Content-Type'],
					})
                '''

		file_name = self.get_data_string()
		path_name = '%s/%s.log' % (RES_FILE_DIR,file_name)

                '''
		fwrite = open(path_name,'a')

		fwrite.write("name=%s\n" % form.getvalue("name",""))
		fwrite.write("addr=%s\n" % form.getvalue("addr",""))
		fwrite.close()
                '''

                length = int(self.headers.getheader('content-length'))
                print "length=" + str(length)
                #data = cgi.parse_qs(self.rfile.read(length), keep_blank_values=1)
                test = self.rfile.read(length)
                print test
                #data = cgi.parse_qs(test, keep_blank_values=1)
                data = cgi.parse_qs(test, keep_blank_values=1)
                #data = self.rfile.read(length)
                print "-----1"
                print data
                print "-----2"

                #f = open("./a.jpg", 'rb')
                self.send_response(200)
                mimetype='image/jpg'
                mimetype='application/json'
                self.send_header('Content-type',mimetype)
                self.end_headers()
                #self.wfile.write(f.read())
                #self.wfile.write("{\"test\":\"adfasd\", \"ws\":\"dfw\"}")
                self.wfile.write(test)
                #f.close()

                '''
		self.send_response(200)
		self.end_headers()
		self.wfile.write("Thanks for you post")
                '''

	def get_data_string(self):
		now = time.time()
		clock_now = time.localtime(now)
		cur_time = list(clock_now)
		date_string = "%d-%d-%d-%d-%d-%d" % (cur_time[0],
				cur_time[1],cur_time[2],cur_time[3],cur_time[4],cur_time[5])
		return date_string		
try:
	server = HTTPServer(('', PORT_NUMBER), myHandler)
	print 'Started httpserver on port ' , PORT_NUMBER

	server.serve_forever()

except KeyboardInterrupt:
	print '^C received, shutting down the web server'
	server.socket.close()
