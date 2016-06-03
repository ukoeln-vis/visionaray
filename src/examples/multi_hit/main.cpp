// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <cassert>
#include <memory>

#include <GL/glew.h>

#include <Support/CmdLine.h>
#include <Support/CmdLineUtil.h>

#include <visionaray/detail/platform.h>

#include <visionaray/bvh.h>
#include <visionaray/camera.h>
#include <visionaray/cpu_buffer_rt.h>
#include <visionaray/get_surface.h>
#include <visionaray/kernels.h>
#include <visionaray/point_light.h>
#include <visionaray/scheduler.h>
#include <visionaray/traverse.h>

#include <common/manip/arcball_manipulator.h>
#include <common/manip/pan_manipulator.h>
#include <common/manip/zoom_manipulator.h>

#include <common/model.h>
#include <common/obj_loader.h>
#include <common/viewer_glut.h>

using namespace visionaray;

using viewer_type = viewer_glut;


//-------------------------------------------------------------------------------------------------
// struct with state variables
//

struct renderer : viewer_type
{
    using host_ray_type = basic_ray<simd::float4>;

    renderer()
        : viewer_type(512, 512, "Visionaray Multi-Hit Example")
        , host_sched(8)
    {
        using namespace support;

        add_cmdline_option( cl::makeOption<std::string&>(
            cl::Parser<>(),
            "filename",
            cl::Desc("Input file in wavefront obj format"),
            cl::Positional,
            cl::Required,
            cl::init(this->filename)
            ) );
    }

    camera                                      cam;
    cpu_buffer_rt<PF_RGBA32F, PF_UNSPECIFIED>   host_rt;
    tiled_sched<host_ray_type>                  host_sched;

    std::string                                 filename;

    model mod;
    index_bvh<model::triangle_list::value_type> host_bvh;

    void create_shading_normals();

protected:

    void on_display();
    void on_resize(int w, int h);

};


//-------------------------------------------------------------------------------------------------
// Brute-force create some interpolated per-vertex triangle normals
// O(n^2) complexity, we have no adjacency info here anymore..
// TODO: do this in model loader!
//

void renderer::create_shading_normals()
{
    assert( mod.geometric_normals.size() == mod.primitives.size() );

    mod.shading_normals.resize(mod.geometric_normals.size() * 3);

    for (size_t i = 0; i < mod.primitives.size(); ++i)
    {
        vec3 n = mod.geometric_normals[i];
        vec3 normals[3] = { n, n, n };

        auto tri1 = mod.primitives[i];
        vec3 verts1[3] = { tri1.v1, tri1.e1 + tri1.v1, tri1.e2 + tri1.v1 };

        for (size_t j = 0; j < mod.primitives.size(); ++j)
        {
            if (i == j)
            {
                continue;
            }

            auto tri2 = mod.primitives[j];
            vec3 verts2[3] = { tri2.v1, tri2.e1 + tri2.v1, tri2.e2 + tri2.v1 };
            vec3 face_normal = mod.geometric_normals[j];

            if (dot(n, face_normal) < 0.5f)
            {
                continue;
            }

            for (size_t k = 0; k < 3; ++k)
            {
                for (size_t l = 0; l < 3; ++l)
                {
                    if (verts1[k] == verts2[l])
                    {
                        normals[k] += face_normal;
                    }
                }
            }
        }

        mod.shading_normals[i * 3]     = normalize(normals[0]);
        mod.shading_normals[i * 3 + 1] = normalize(normals[1]);
        mod.shading_normals[i * 3 + 2] = normalize(normals[2]);
    }
}


//-------------------------------------------------------------------------------------------------
// Display function, implements the multi-hit kernel
//

void renderer::on_display()
{
    // some setup

    using R = renderer::host_ray_type;
    using S = R::scalar_type;
    using C = vector<4, S>;
    using V = vector<3, S>;
    using L = point_light<float>;

    auto sparams = make_sched_params(
            pixel_sampler::uniform_type{},
            cam,
            host_rt
            );


    using bvh_ref = index_bvh<model::triangle_list::value_type>::bvh_ref;

    std::vector<bvh_ref> bvhs;
    bvhs.push_back(host_bvh.ref());

    auto bgcolor = background_color();

    aligned_vector<L> lights;

    L light;
    light.set_cl( vec3(1.0, 1.0, 1.0) );
    light.set_kl(1.0);
    light.set_position( cam.eye() );

    lights.push_back(light);

    auto params = make_kernel_params(
            normals_per_vertex_binding{},
            bvhs.data(),
            bvhs.data() + bvhs.size(),
            mod.shading_normals.data(),
            mod.materials.data(),
            lights.data(),
            lights.data() + lights.size()
            );

    host_sched.frame([&](R ray) -> result_record<S>
    {
        result_record<S> result;
        result.color = C(0.0);

        // Perform multi-hit, we allow for up to 16 hits
        // Multi-hit returns a sorted array (based on
        // ray parameter "t") of hit records
        auto hit_rec = multi_hit<16>(
                ray,
                params.prims.begin,
                params.prims.end
                );

        // Use closest hit in the sequence
        // for visibility testing
        result.hit = hit_rec[0].hit;
        result.isect_pos  = ray.ori + ray.dir * hit_rec[0].t;


        // Do smth. useful with the hit records
        for (size_t i = 0; i < hit_rec.size(); ++i)
        {
            if (!any(hit_rec[i].hit))
            {
                break;
            }

            hit_rec[i].isect_pos  = ray.ori + ray.dir * hit_rec[i].t;


            // Surface shading
            auto surf = get_surface(hit_rec[i], params);
            auto view_dir = -ray.dir;

            auto n = surf.shading_normal;

#if 1 // two-sided
            n = faceforward( n, view_dir, surf.normal );
#endif

            auto it = params.lights.begin;
            auto sr         = make_shade_record<decltype(params), S>();
            sr.active       = hit_rec[i].hit;
            sr.isect_pos    = hit_rec[i].isect_pos;
            sr.normal       = n;
            sr.view_dir     = view_dir;
            sr.light_dir    = normalize( V(it->position()) - hit_rec[i].isect_pos );
            sr.light        = *it;
            auto shaded_clr = surf.shade(sr);

            // Front-to-back alpha compositing
            auto color      = to_rgba(shaded_clr);
            color.w         = S(0.3);
            auto premult    = color.xyz() * color.w;
            color           = C(premult, color.w);

            result.color += select(
                    hit_rec[i].hit,
                    color * (1.0f - result.color.w),
                    C(0.0)
                    );
        }

        return result;
    }, sparams);


    // display the rendered image

    glClearColor(bgcolor.x, bgcolor.y, bgcolor.z, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    host_rt.display_color_buffer();
}


//-------------------------------------------------------------------------------------------------
// resize event
//

void renderer::on_resize(int w, int h)
{
    cam.set_viewport(0, 0, w, h);
    float aspect = w / static_cast<float>(h);
    cam.perspective(45.0f * constants::degrees_to_radians<float>(), aspect, 0.001f, 1000.0f);
    host_rt.resize(w, h);

    viewer_type::on_resize(w, h);
}


//-------------------------------------------------------------------------------------------------
// Main function, performs initialization
//

int main(int argc, char** argv)
{
    renderer rend;

    try
    {
        rend.init(argc, argv);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    try
    {
        visionaray::load_obj(rend.filename, rend.mod);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed loading obj model: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (rend.mod.shading_normals.size() == 0)
    {
        std::cout << "Creating shading normals...\n";
        rend.create_shading_normals();
    }

    std::cout << "Creating BVH...\n";

    rend.host_bvh = build<index_bvh<model::triangle_list::value_type>>(
            rend.mod.primitives.data(),
            rend.mod.primitives.size()
            );

    std::cout << "Ready\n";

    float aspect = rend.width() / static_cast<float>(rend.height());

    rend.cam.perspective(45.0f * constants::degrees_to_radians<float>(), aspect, 0.001f, 1000.0f);
    rend.cam.view_all( rend.mod.bbox );

    rend.add_manipulator( std::make_shared<arcball_manipulator>(rend.cam, mouse::Left) );
    rend.add_manipulator( std::make_shared<pan_manipulator>(rend.cam, mouse::Middle) );
    // Additional "Alt + LMB" pan manipulator for setups w/o middle mouse button
    rend.add_manipulator( std::make_shared<pan_manipulator>(rend.cam, mouse::Left, keyboard::Alt) );
    rend.add_manipulator( std::make_shared<zoom_manipulator>(rend.cam, mouse::Right) );

    rend.event_loop();
}