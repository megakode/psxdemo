import sys
import os
import struct
import bpy
import bmesh
import mathutils

from .file_writer import FileWriter

class Vertex:
	def __init__(self):
		self.position = mathutils.Vector()
		self.normal = mathutils.Vector()
		self.uv = mathutils.Vector()
		self.boneIndex = [0,0,0,0]
		self.boneWeights = [0,0,0,0]
	
	def __eq__(self, other):
		return (self.position == other.position and self.normal == other.normal and self.uv == other.uv)
	
	def __hash__(self):
		return hash((self.position.x, self.position.y, self.position.z, self.normal.x, self.normal.y, self.normal.z, self.uv.x, self.uv.y))
		
class SceneExporter:
	writer = None
	option_triangulate = True
	option_onlySelected = True
	option_animations = True
	option_animationLoop = False
	
	#Meshes
	meshMap = dict()
	globalMeshCount = 0
	
	#Materials
	materialMap = dict()
	globalMaterialCount = 0

	def export(self, filePath):
		#Write header
		self.writer = FileWriter(filePath)
		self.writer.beginChunk('ROOT')
		self.writer.addInt32('VERS', 1)

		#Write data
		self.writeMaterials()
		self.writeMeshes()
		self.writeNodes()
		self.writeAnimations()
		
		#End
		self.writer.endChunk()
	
	def shouldWriteObject(self, object):
		#Check if active
		if(object.hide):
			return False
		#if(self.option_onlySelected and not object.select):
		#	return False
			
		#Yes we can write
		return True

	def writeNodes(self):
	
		#Write nodes
		for object in bpy.context.scene.objects:

			#Check if root object
			if(object.parent):
				continue
				
			#Write node
			self.writeNode(object)
				
	def writeNode(self, node):
	
		#Check if should write
		if(not self.shouldWriteObject(node)):
			return
		
		#Begin NODE
		self.writer.beginChunk('NODE')
		
		#Base data
		self.writer.addText('NAME', node.name)
		self.writer.addVec('ORIG', node.matrix_local.to_translation())
		self.writer.addQuat('ROTA', node.matrix_local.to_quaternion())
		self.writer.addVec('SCAL', node.matrix_local.to_scale())
		
		#Mesh
		if(node.type == 'MESH'):
			#Find mesh index
			for meshIndex in self.meshMap[node]:
				self.writer.addInt32('MESH', meshIndex)
				
		#Children
		for child in node.children:
			self.writeNode(child)
			
		#Bones
		if(node.type == 'ARMATURE'):
		
			#Set to rest position
			node.data.pose_position = "REST"
			bpy.context.scene.update()
		
			#Write skeleton
			self.writer.beginChunk('SKEL')
			self.writer.addInt32('SIZE', len(node.pose.bones))
			for bone in node.pose.bones:
				#Begin BONE
				self.writer.beginChunk('BONE')
			
				#Name
				self.writer.addText('NAME', bone.name)
				
				#Rest chunk
				self.writer.beginChunk('REST')
				matrix = bone.matrix
				self.writer.addVec('ORIG', matrix.to_translation())
				self.writer.addQuat('ROTA', matrix.to_quaternion())
				self.writer.addVec('SCAL', matrix.to_scale())
				self.writer.endChunk()
				
				#End BONE
				self.writer.endChunk()
				
			self.writer.endChunk()
			
			#Write bone nodes
			for bone in node.pose.bones:
				#Check if root level
				if(bone.parent):
					continue
					
				#Process
				self.writeNode_Bone(bone)
		
		#End NODE
		self.writer.endChunk()
		
	def writeNode_Bone(self, bone):
		
		#Begin NODE
		self.writer.beginChunk('NODE')
		
		#Base data
		self.writer.addText('NAME', bone.name)
		
		#Find position
		matrix = self.getLocalMatrix_Bone(bone)			
		self.writer.addVec('ORIG', matrix.to_translation())
		self.writer.addQuat('ROTA', matrix.to_quaternion())
		self.writer.addVec('SCAL', matrix.to_scale())
		
		#Children
		for child in bone.children:
			self.writeNode_Bone(child)
		
		#End NODE
		self.writer.endChunk()
		
	def writeMaterials(self):
		
		for object in bpy.context.scene.objects:
		
			print("Checking object for materials:" + object.name)
			
			#Check type
			if(object.type != 'MESH'):
				continue
				
			print("Valid 1")
				
			if(not self.shouldWriteObject(object)):
				continue
				
			print("Valid 2")
		
			for material in object.data.materials:
				
				#Check if already written
				if(self.materialMap.get(material)):
					continue
				self.materialMap[material] = self.globalMaterialCount
				self.globalMaterialCount += 1
				
				print("Material:" + material.name)
			
				#Begin MATR
				self.writer.beginChunk('MATR')
				
				#Data
				self.writer.addText('NAME', material.name)
				if(material.use_transparency):
					self.writer.addUInt8("TNSP", 1);
				if(not material.use_cast_shadows):
					self.writer.addUInt8("SHDC", int(material.use_cast_shadows))
				
				#Check for textures
				for slot in material.texture_slots:
				
					#Check slot
					if(not slot):
						continue
				
					#Check if used
					if(not slot.texture or slot.texture.type != 'IMAGE'):
						continue
						
					#Check if source if file
					image = slot.texture.image
					if(not image or image.source != 'FILE'):
						continue
				
					#Begin TEXR
					self.writer.beginChunk('TEXR')
					
					#File path
					filepath = image.filepath
					if(filepath.find("//") == 0):
						filepath = filepath[2:len(filepath)]
					self.writer.addText('FILE', filepath)
					
					#Type
					if(slot.use_map_color_diffuse):
						self.writer.addText('TYPE', "color")
					elif(slot.use_map_mirror):
						self.writer.addText('TYPE', "metal")
					elif(slot.use_map_color_spec):
						self.writer.addText('TYPE', "rough")
					elif(slot.use_map_normal):
						self.writer.addText('TYPE', "normal")
					
					#End TEXR
					self.writer.endChunk()
				
				#End MATR
				self.writer.endChunk()
			
		#Return
		return True

	def writeMeshes(self):
		
		for object in bpy.context.scene.objects:
		
			#Check if should write
			if(not self.shouldWriteObject(object)):
				continue
		
			#Check type
			if(object.type == 'MESH'):
				self.writeObjectMeshs(object)
		
	def writeObjectMeshs(self, object):
	
		print("Writing Object Meshes:" + object.name)
	
		#Disable armature
		armatureMod = object.modifiers.get('Armature')
		if(armatureMod):
			armatureMod.object.data.pose_position = "REST"
			bpy.context.scene.update()
		
		#Create export mesh
		applyTriangulate = (object.modifiers.get('Triangulate') == None)
		if(applyTriangulate):
			bpy.context.scene.objects.active = object
			bpy.ops.object.modifier_add(type='TRIANGULATE')
		mesh = object.to_mesh(bpy.context.scene, True, 'RENDER')
		
		#Check if linked to object
		print("Mesh:" + mesh.name)
		print("Materials:" + str(len(mesh.materials)))
		
		mesh.calc_normals_split()
		uvLayer = mesh.uv_layers.active
		
		#Get bones
		boneList = None
		if(object.parent and object.parent.type == "ARMATURE"):
			boneList = object.parent.pose.bones
			
		#For each material
		meshList = list()
		materialIndex = 0
		for material in mesh.materials:
		
			#Data
			vertMap = dict()
			vertexList = list()
			indexList = list()
		
			#For each face
			for face in mesh.polygons:
				
				#Are we of this material
				if materialIndex != face.material_index:
					continue
				
				#For each loop
				for loopIndex in face.loop_indices:
				
					loop = mesh.loops[loopIndex]
					
					#Define vertex
					vertex = Vertex()
					vertex.position = mesh.vertices[loop.vertex_index].co
					vertex.normal = loop.normal
					
					#UV Coords
					if(uvLayer != None):
						vertex.uv = uvLayer.data[loopIndex].uv
					
					#Weights
					self.getVertexBoneWeights(object, mesh.vertices[loop.vertex_index], vertex)
					
					#Sanitize data (Blender is really fucking poor about these numbers)
					#We go to the 5th decimal place, this seems to be where the numbers start to diverge
					#Without this, the map loopup doesn't function properly
					vertex.position.x = round(vertex.position.x, 5)
					vertex.position.y = round(vertex.position.y, 5)
					vertex.position.z = round(vertex.position.z, 5)
					vertex.normal.x = round(vertex.normal.x, 5)
					vertex.normal.y = round(vertex.normal.y, 5)
					vertex.normal.z = round(vertex.normal.z, 5)
					vertex.uv.x = round(vertex.uv.x, 5)
					vertex.uv.y = round(vertex.uv.y, 5)
					
					#Check if it exists, if not store it
					vertexIndex = vertMap.get(vertex)
					if(vertexIndex == None):
						vertexIndex = len(vertMap)
						vertMap[vertex] = vertexIndex
						vertexList.append(vertex)
						
					#Add index
					indexList.append(vertexIndex)
					
			#Find global material index
			globalMatIndex = self.materialMap[mesh.materials[materialIndex]]
			
			#Write vertex data to disk
			self.writeMesh(vertexList, indexList, globalMatIndex, boneList)
			
			#Increment material
			materialIndex += 1
			
			#Increment mesh
			meshList.append(self.globalMeshCount)
			self.globalMeshCount += 1
			
		#Cleanup
		bpy.data.meshes.remove(mesh)
		
		#Remove modifier
		if(applyTriangulate):
			bpy.ops.object.modifier_remove(modifier='Triangulate')
		
		#Add to mesh map
		self.meshMap[object] = meshList

	def writeMesh(self, vertexList, indexList, materialIndex, boneList):
	
		print("Writting Mesh - Verts:" + str(len(vertexList)) + " Faces:" + str(len(indexList)/3))
		
		#Begin MESH
		self.writer.beginChunk('MESH')
		
		#Data
		self.writer.addInt32('MATR', materialIndex);

		#Begin VERT
		vertSize = len(vertexList)
		self.writer.beginChunk('VERT')
		self.writer.addInt32('SIZE', vertSize)

		#Positions
		self.writer.beginChunk('CHAN')
		self.writer.addText('TYPE', "position")

		self.writer.beginChunk('DATA')
		for vert in vertexList:
			   self.writer.stream.write(struct.pack("<fff", vert.position.x, vert.position.y, vert.position.z))
		self.writer.endChunk()

		self.writer.endChunk()

		#Normals
		self.writer.beginChunk('CHAN')
		self.writer.addText('TYPE', "normal")

		self.writer.beginChunk('DATA')
		for vert in vertexList:
			self.writer.stream.write(struct.pack("<fff", vert.normal.x, vert.normal.y, vert.normal.z))
		self.writer.endChunk()
			
		self.writer.endChunk()
		
		#UV
		self.writer.beginChunk('CHAN')
		self.writer.addText('TYPE', "uv")

		self.writer.beginChunk('DATA')
		for vert in vertexList:
			self.writer.stream.write(struct.pack("<ff", vert.uv.x, vert.uv.y))
		self.writer.endChunk()
			
		self.writer.endChunk()
		
		#Bone Weights
		if(boneList):
			self.writer.beginChunk('CHAN')
			self.writer.addText('TYPE', "bone_weight")
			
			self.writer.beginChunk('DATA')
			for vert in vertexList:
				self.writer.stream.write(struct.pack("<iiii", vert.boneIndex[0], vert.boneIndex[1], vert.boneIndex[2], vert.boneIndex[3]))
				self.writer.stream.write(struct.pack("<ffff", vert.boneWeights[0], vert.boneWeights[1], vert.boneWeights[2], vert.boneWeights[3]))
			self.writer.endChunk()
			
			#End CHAN
			self.writer.endChunk()

		#End VERT
		self.writer.endChunk()

		#Begin FACE
		self.writer.beginChunk('FACE')

		faceSize = int(len(indexList)/3)
		self.writer.addInt32('SIZE', faceSize)
		self.writer.beginChunk('DATA')
		for index in indexList:
			self.writer.stream.write(struct.pack("<i", index))
		self.writer.endChunk()
			
		#End FACE
		self.writer.endChunk()
		
		#Bones
		if(boneList):
		
			#Begin BONE
			self.writer.beginChunk('BONE')
			self.writer.addInt32('SIZE', len(boneList))
			for bone in boneList:
				self.writer.addText('NODE', bone.name)
				
			#End BONE
			self.writer.endChunk()

		#End MESH
		self.writer.endChunk()
		
	def writeAnimations(self):
	
		#Check option_animations
		if(not self.option_animations):
			return
		
		#Data
		scene = bpy.context.scene
		prevFrame = scene.frame_current
		
		#Begin ANIM
		self.writer.beginChunk("ANIM")
		self.writer.addText('ANIM', "Scene");
		self.writer.addFloat32('STRT', scene.frame_start)
		self.writer.addFloat32('STOP', scene.frame_end)
		self.writer.addFloat32('TMSC', 1.0/scene.render.fps)
		self.writer.addInt8('LOOP', int(self.option_animationLoop))
		
		#Write object layers
		for object in bpy.context.scene.objects:
			
			#Write layer
			self.writeAnimationLayer_Object(object)
			
			#Write bones
			if(object.type == 'ARMATURE'):
				for bone in object.pose.bones:
					self.writeAnimationLayer_Bone(object, bone)
			
		#End ANIM
		self.writer.endChunk()
		
		#Reset scene
		scene.frame_set(prevFrame)
		
	def writeAnimationLayer_Object(self, object):
		
		scene = bpy.context.scene
		
		#Compile keyframe data
		keyCount = (scene.frame_end+1)-scene.frame_start
		
		positionList = list()
		rotationList = list()
		scaleList = list()
		
		positionChanged = False
		rotationChanged = False
		scaleChanged = False
		
		prevPosition = object.matrix_local.to_translation()
		prevRotation = object.matrix_local.to_quaternion()
		prevScale = object.matrix_local.to_scale()
		
		for frame in range(scene.frame_start, scene.frame_end+1):
			#Advance scene
			scene.frame_set(frame)
			
			#Get object position data
			matrix = object.matrix_local
			position = matrix.to_translation()
			rotation = matrix.to_quaternion()
			scale = matrix.to_scale()
			
			#Append
			positionList.append(position)
			rotationList.append(rotation)
			scaleList.append(scale)
			
			#Check for change
			if(position != prevPosition):
				positionChanged = True
			if(rotation != prevRotation):
				rotationChanged = True
			if(scale != prevScale):
				scaleChanged = True
			
		#Check for change
		if(not positionChanged and not rotationChanged and not scaleChanged):
			return
			
		#Begin LAYR
		self.writer.beginChunk('LAYR')
		self.writer.addText('NODE', object.name)
		
		#Position Channel
		if(positionChanged):
			
			#Begin CHAN
			self.writer.beginChunk("CHAN")
			self.writer.addText('TYPE', "position_baked")
			self.writer.addInt32('KYNM', keyCount)
			
			#Keys
			self.writer.beginChunk('KEYS')
			for key in positionList:
				self.writer.stream.write(struct.pack("<fff", key.x, key.y, key.z))
			self.writer.endChunk()
			
			#End CHAN
			self.writer.endChunk()
			
		#Rotation Channel
		if(rotationChanged):
			
			#Begin CHAN
			self.writer.beginChunk("CHAN")
			self.writer.addText('TYPE', "rotation_baked")
			self.writer.addInt32('KYNM', keyCount)
			
			#Keys
			self.writer.beginChunk('KEYS')
			for key in rotationList:
				self.writer.stream.write(struct.pack("<ffff", key.w, key.x, key.y, key.z))
			self.writer.endChunk()
			
			#End CHAN
			self.writer.endChunk()
			
		#Scale Channel
		if(scaleChanged):
			
			#Begin CHAN
			self.writer.beginChunk("CHAN")
			self.writer.addText('TYPE', "scale_baked")
			self.writer.addInt32('KYNM', keyCount)
			
			#Keys
			self.writer.beginChunk('KEYS')
			for key in scaleList:
				self.writer.stream.write(struct.pack("<fff", key.x, key.y, key.z))
			self.writer.endChunk()
			
			#End CHAN
			self.writer.endChunk()
		
		#End LAYR
		self.writer.endChunk()
		
	def writeAnimationLayer_Bone(self, armature, object):
		
		scene = bpy.context.scene
		
		#Compile keyframe data
		keyCount = (scene.frame_end+1)-scene.frame_start
		
		positionList = list()
		rotationList = list()
		scaleList = list()
		
		positionChanged = False
		rotationChanged = False
		scaleChanged = False
		
		#Move into rest pose
		armature.data.pose_position = "REST"
		bpy.context.scene.update()
		
		#Initialize with rest position
		matrix = self.getLocalMatrix_Bone(object)
		prevPosition = matrix.to_translation()
		prevRotation = matrix.to_quaternion()
		prevScale = matrix.to_scale()
		
		#Move back to pose position
		armature.data.pose_position = "POSE"
		bpy.context.scene.update()
		
		for frame in range(scene.frame_start, scene.frame_end+1):
			#Advance scene
			scene.frame_set(frame)
			
			#Get object position data
			matrix = self.getLocalMatrix_Bone(object)
			position = matrix.to_translation()
			rotation = matrix.to_quaternion()
			scale = matrix.to_scale()
			
			#Append
			positionList.append(position)
			rotationList.append(rotation)
			scaleList.append(scale)
			
			#Check for change
			if(position != prevPosition):
				positionChanged = True
			if(rotation != prevRotation):
				rotationChanged = True
			if(scale != prevScale):
				scaleChanged = True
			
		#Check for change
		if(not positionChanged and not rotationChanged and not scaleChanged):
			return
			
		#Begin LAYR
		self.writer.beginChunk('LAYR')
		self.writer.addText('NODE', object.name)
		
		#Position Channel
		if(positionChanged):
			
			#Begin CHAN
			self.writer.beginChunk("CHAN")
			self.writer.addText('TYPE', "position_baked")
			self.writer.addInt32('KYNM', keyCount)
			
			#Keys
			self.writer.beginChunk('KEYS')
			for key in positionList:
				self.writer.stream.write(struct.pack("<fff", key.x, key.y, key.z))
			self.writer.endChunk()
			
			#End CHAN
			self.writer.endChunk()
			
		#Rotation Channel
		if(rotationChanged):
			
			#Begin CHAN
			self.writer.beginChunk("CHAN")
			self.writer.addText('TYPE', "rotation_baked")
			self.writer.addInt32('KYNM', keyCount)
			
			#Keys
			self.writer.beginChunk('KEYS')
			for key in rotationList:
				self.writer.stream.write(struct.pack("<ffff", key.w, key.x, key.y, key.z))
			self.writer.endChunk()
			
			#End CHAN
			self.writer.endChunk()
			
		#Scale Channel
		if(scaleChanged):
			
			#Begin CHAN
			self.writer.beginChunk("CHAN")
			self.writer.addText('TYPE', "scale_baked")
			self.writer.addInt32('KYNM', keyCount)
			
			#Keys
			self.writer.beginChunk('KEYS')
			for key in scaleList:
				self.writer.stream.write(struct.pack("<fff", key.x, key.y, key.z))
			self.writer.endChunk()
			
			#End CHAN
			self.writer.endChunk()
		
		#End LAYR
		self.writer.endChunk()
		
	def getLocalMatrix_Bone(self, bone):
		if(bone.parent == None):
			return bone.matrix.copy()
		else:
			return bone.parent.matrix.inverted() * bone.matrix
	
	def getVertexBoneWeights(self, object, mesh_vertex, vertex_out):
	
		#Get armature
		if(not object.parent or object.parent.type != 'ARMATURE'):
			return
		armature = object.parent
		
		#Loop through groups
		boneCount = 0
		for group in mesh_vertex.groups:
			
			#Find bone associated with this group
			vertGroup = object.vertex_groups[group.group];
			bone = armature.pose.bones.get(vertGroup.name)
			if(not bone):
				continue
			
			#Check if deform
			otherBone = armature.data.bones.get(bone.name);
			if(not otherBone or not otherBone.use_deform):
				continue
				
			#Check num of bones
			if(boneCount >= 4):
				continue
			
			#Find bone index
			boneFound = False
			boneIndex = 0
			for bone in armature.pose.bones:
				if(bone.name == vertGroup.name):
					boneFound = True
					break
				boneIndex += 1
			if(not boneFound):
				continue
				
			#Minimum weight
			weight = round(group.weight, 5);
			if(weight <= 0):
				continue
				
			#Store data
			vertex_out.boneIndex[boneCount] = boneIndex
			vertex_out.boneWeights[boneCount] = weight
			
			#Increment
			boneCount += 1
			
		#Normalize bone data
		if(boneCount > 0):
		
			#Find total weight
			totalWeight = 0
			for i in range(0,boneCount):
				totalWeight += vertex_out.boneWeights[i]
			weightMulti = 1.0/totalWeight
			
			#Modify weights
			for i in range(0,boneCount):
				vertex_out.boneWeights[i] *= weightMulti
				
	