import bpy
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, EnumProperty, BoolProperty

from .exporter import SceneExporter

bl_info = {
	"name":			"Forge Quest Model",
	"author":			"Chase Grozdina",
	"blender":		(2,7,0),
	"version":		(1,0),
	"location":		"File > Export",
	"description":	"",
	"category":		"Import-Export"
}

class MyExporter(bpy.types.Operator, ExportHelper):
	bl_idname	= "forge_quest_model.mdl";
	bl_label		= "Forge Quest Model Exporter";
	bl_options	= {'PRESET'};
	filename_ext	= ".mdl";
	
	#option_onlySelected = BoolProperty(name="Only selected", default=True, description="What object will be exported? Only selected / all objects")
	option_animations = BoolProperty(name="Include Animations", default=True, description="Are animations included in the export file?")
	option_animationLoop = BoolProperty(name="Animation Loop", default=False, description="Does the animation loop in engine?")
	
	def execute(self, context):
		
		#File Path
		filePath = bpy.path.ensure_ext(self.filepath, ".mdl")
		
		#Convert scene to game file format
		exporter = SceneExporter()
		#exporter.option_onlySelected = self.option_onlySelected;
		exporter.option_animations = self.option_animations;
		exporter.option_animationLoop = self.option_animationLoop;
		exporter.export(filePath)
		
		return {'FINISHED'};
	
# Define the Blender required registration functions.
def menu_func(self, context):
	self.layout.operator(MyExporter.bl_idname, text="Forge Quest Model(.mdl)");

def register():
	bpy.utils.register_module(__name__);
	bpy.types.INFO_MT_file_export.append(menu_func);
	
def unregister():
	bpy.utils.unregister_module(__name__);
	bpy.types.INFO_MT_file_export.remove(menu_func);
	
if __name__ == "__main__":
	register()