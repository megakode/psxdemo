import sys
import os
import struct
import bpy
import bmesh
import mathutils

class FileWriter:
	stream = None
	chunkStack = list()

	class Chunk:
		tag = 0;
		position = 0;
	
	def __init__(self, filePath):
		self.stream = open(filePath, 'wb');
	def __del__(self):
		self.stream.close()
		
	def beginChunk(self, tag):
		#Push chunk onto stack
		chunk = FileWriter.Chunk()
		chunk.tag = bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])])
		chunk.position = self.stream.tell()
		self.chunkStack.append(chunk)
		
		#Write
		self.stream.seek(8, os.SEEK_CUR);
		
	def endChunk(self):
		#Pop chunk off stack
		stackSize = len(self.chunkStack)
		chunk = self.chunkStack.pop()
		
		#Move to start of chunk
		prevPos = self.stream.tell()
		delta = prevPos-chunk.position;
		self.stream.seek(-delta, os.SEEK_CUR)
		
		#Write tag and size
		self.stream.write(chunk.tag)
		self.stream.write(struct.pack("<i", delta-8))
		
		#Move back to position
		self.stream.seek(prevPos)


	def addInt8(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 1) )
		self.stream.write( struct.pack("<b", value) )

	def addInt16(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 2) )
		self.stream.write( struct.pack("<h", value) )
		
	def addInt32(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 4) )
		self.stream.write( struct.pack("<i", value) )

	def addInt64(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 8) )
		self.stream.write( struct.pack("<q", value) )

	def addUInt8(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 1) )
		self.stream.write( struct.pack("<B", value) )

	def addUInt16(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 2) )
		self.stream.write( struct.pack("<H", value) )
		
	def addUInt32(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 4) )
		self.stream.write( struct.pack("<I", value) )

	def addUInt64(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 8) )
		self.stream.write( struct.pack("<Q", value) )

	def addFloat32(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 4) )
		self.stream.write( struct.pack("<f", value) )

	def addFloat64(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 8) )
		self.stream.write( struct.pack("<d", value) )

	def addText(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", len(value)) )
		self.stream.write( bytes(value, 'utf-8') )
		
	def addVec(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 12) )
		self.stream.write( struct.pack("<fff", value.x, value.y, value.z) )
		
	def addQuat(self, tag, value):
		self.stream.write( bytes([ord(tag[0]), ord(tag[1]), ord(tag[2]), ord(tag[3])]) )
		self.stream.write( struct.pack("<i", 16) )
		self.stream.write( struct.pack("<ffff", value.w, value.x, value.y, value.z) )