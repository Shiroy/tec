#include "os.hpp"
#include "event-system.hpp"
#include "event-queue.hpp"
#include "render-system.hpp"
#include "vertexbuffer.hpp"
#include "shader.hpp"
#include "voxelvolume.hpp"
#include "components/transforms.hpp"
#include "material.hpp"
#include "entity.hpp"
#include "components/camera.hpp"
#include "polygonmeshdata.hpp"
#include "component-update-system.hpp"
#include <glm/gtc/matrix_transform.hpp>

std::list<std::function<void(tec::frame_id_t)>> tec::ComponentUpdateSystemList::update_funcs;

int main(int argc, char* argv[]) {
	tec::OS os;

	os.InitializeWindow(800, 600, "TEC 0.1", 3, 2);

	tec::RenderSystem rs;

	rs.SetViewportSize(800, 600);

	auto voxvol = tec::VoxelVolume::Create(100, "bob", 0);
	auto voxvol_shared = voxvol.lock();
	auto shader_files = std::list < std::pair<tec::Shader::ShaderType, std::string> > {
		std::make_pair(tec::Shader::VERTEX, "assets/basic.vert"), std::make_pair(tec::Shader::FRAGMENT, "assets/basic.frag"),
	};
	auto s = tec::Shader::CreateFromFile("shader1", shader_files);
	auto basic_fill = tec::Material::Create("material_basic", s);

	shader_files = std::list < std::pair<tec::Shader::ShaderType, std::string> > {
		std::make_pair(tec::Shader::VERTEX, "assets/basic.vert"), std::make_pair(tec::Shader::FRAGMENT, "assets/overlay.frag"),
	};
	auto s_overlay = tec::Shader::CreateFromFile("shader_overlay", shader_files);
	auto overlay = tec::Material::Create("material_overlay", s_overlay);
	overlay.lock()->SetFillMode(GL_LINE);

	tec::ComponentUpdateSystem<tec::Position>::Initialize();
	tec::ComponentUpdateSystem<tec::Orientation>::Initialize();
	tec::ComponentUpdateSystem<tec::Camera>::Initialize();
	tec::ComponentUpdateSystem<tec::Renderable>::Initialize();
	tec::ComponentUpdateSystem<tec::View>::Initialize();

	tec::Entity voxel1(100);
	voxel1.Add<tec::Position>();
	voxel1.Add<tec::Orientation>();
	voxel1.Add<tec::Renderable>();

	tec::VoxelCommand add_voxel(
		[ ] (tec::VoxelVolume* vox_vol) {
		vox_vol->AddVoxel(0, 1, 1);
		vox_vol->AddVoxel(0, -1, 1);
		vox_vol->AddVoxel(0, -1, 0);
		vox_vol->AddVoxel(0, -1, -1);
		vox_vol->AddVoxel(1, -1, 1);
	});
	tec::VoxelVolume::QueueCommand(std::move(add_voxel));
	voxvol_shared->Update(0.0);

	tec::RenderCommand add_vb(
		[voxvol_shared, basic_fill, overlay] (tec::RenderSystem* ren_sys) {
		auto mesh = voxvol_shared->GetMesh().lock();
		if (mesh) {
			auto vb = std::make_shared<tec::VertexBuffer>();
			tec::VertexBufferMap::Set(100, vb);
			vb->Buffer(*mesh->GetVertexBuffer(), *mesh->GetIndexBuffer());

			ren_sys->AddVertexBuffer(basic_fill, vb, 100);
			ren_sys->AddVertexBuffer(overlay, vb, 100);

			auto vb2 = std::make_shared<tec::VertexBuffer>();
			tec::VertexBufferMap::Set(1, vb2);
			vb2->Buffer(*mesh->GetVertexBuffer(), *mesh->GetIndexBuffer());

			ren_sys->AddVertexBuffer(basic_fill, vb2, 1);
		}
	});
	tec::RenderSystem::QueueCommand(std::move(add_vb));

	tec::Entity camera(1);
	camera.Add<tec::Position>();
	camera.Add<tec::Orientation>();
	camera.Add<tec::Camera>(1);
	camera.Add<tec::Renderable>();
	tec::Entity camera2(2);
	camera2.Add<tec::Position>();
	camera2.Add<tec::Orientation>();
	camera2.Add<tec::Camera>(2);

	tec::CameraMover cam_mover(1);
	
    std::int64_t frame_id = 1;

	while (!os.Closing()) {
		tec::ComponentUpdateSystemList::UpdateAll(frame_id);

		cam_mover.Update(0.0);
		rs.Update(os.GetDeltaTime());
		os.OSMessageLoop();
		os.SwapBuffers();
		frame_id++;
	}

	return 0;
}
