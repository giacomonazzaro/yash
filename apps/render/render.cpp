#include <yocto/yocto_cli.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_parallel.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_trace.h>

#if YOCTO_OPENGL == 1
#include <yocto_gui/yocto_glview.h>
#endif

#include "scene_hash.h"
#include "scene_view.h"

using namespace yocto;
using namespace yash;

// convert params
struct convert_params {
  string scene     = "scene.ply";
  string output    = "out.ply";
  bool   info      = false;
  bool   validate  = false;
  string copyright = "";
};

// Cli
void add_options(const cli_command& cli, convert_params& params) {
  add_argument(cli, "scene", params.scene, "Input scene.");
  add_option(cli, "output", params.output, "Output scene.");
  add_option(cli, "info", params.info, "Print info.");
  add_option(cli, "validate", params.validate, "Validate scene.");
  add_option(cli, "copyright", params.copyright, "Set scene copyright.");
}

// convert images
void run_convert(const convert_params& params) {
  // load scene
  auto error = string{};
  auto scene = scene_data{};
  print_progress_begin("load scene");
  if (!load_scene(params.scene, scene, error)) print_fatal(error);
  print_progress_end();

  // copyright
  if (params.copyright != "") {
    scene.copyright = params.copyright;
  }

  // validate scene
  if (params.validate) {
    for (auto& error : scene_validation(scene)) print_info("error: " + error);
  }

  // print info
  if (params.info) {
    print_info("scene stats ------------");
    for (auto stat : scene_stats(scene)) print_info(stat);
  }

  // tesselate if needed
  if (!scene.subdivs.empty()) {
    print_progress_begin("tesselate subdivs");
    tesselate_subdivs(scene);
    print_progress_end();
  }

  // save scene
  print_progress_begin("save scene");
  make_scene_directories(params.output, scene);
  save_scene(params.output, scene);
  print_progress_end();
}

// info params
struct info_params {
  string scene    = "scene.ply";
  bool   validate = false;
};

// Cli
void add_options(const cli_command& cli, info_params& params) {
  add_argument(cli, "scene", params.scene, "Input scene.");
  add_option(cli, "validate", params.validate, "Validate scene.");
}

// print info for scenes
void run_info(const info_params& params) {
  // load scene
  auto error = string{};
  print_progress_begin("load scene");
  auto scene = scene_data{};
  if (!load_scene(params.scene, scene, error)) print_fatal(error);
  print_progress_end();

  // validate scene
  if (params.validate) {
    for (auto& error : scene_validation(scene)) print_info("error: " + error);
  }

  // print info
  print_info("scene stats ------------");
  for (auto stat : scene_stats(scene)) print_info(stat);
}

// render params
struct render_params : trace_params {
  string scene     = "scene.json";
  string output    = "out.png";
  string camname   = "";
  bool   addsky    = false;
  string envname   = "";
  bool   savebatch = false;
};

// Cli
void add_options(const cli_command& cli, render_params& params) {
  add_argument(cli, "scene", params.scene, "Scene filename.");
  add_option(cli, "output", params.output, "Output filename.");
  add_option(cli, "camera", params.camname, "Camera name.");
  add_option(cli, "addsky", params.addsky, "Add sky.");
  add_option(cli, "envname", params.envname, "Add environment map.");
  add_option(cli, "savebatch", params.savebatch, "Save batch.");
  add_option(
      cli, "resolution", params.resolution, "Image resolution.", {1, 4096});
  add_option(
      cli, "sampler", params.sampler, "Sampler type.", trace_sampler_names);
  add_option(cli, "falsecolor", params.falsecolor, "False color type.",
      trace_falsecolor_names);
  add_option(cli, "samples", params.samples, "Number of samples.", {1, 4096});
  add_option(cli, "bounces", params.bounces, "Number of bounces.", {1, 128});
  add_option(cli, "denoise", params.denoise, "Enable denoiser.");
  add_option(cli, "batch", params.batch, "Sample batch.");
  add_option(cli, "clamp", params.clamp, "Clamp params.", {10, flt_max});
  add_option(cli, "nocaustics", params.nocaustics, "Disable caustics.");
  add_option(cli, "envhidden", params.envhidden, "Hide environment.");
  add_option(cli, "tentfilter", params.tentfilter, "Filter image.");
  add_option(cli, "embreebvh", params.embreebvh, "Use Embree as BVH.");
  add_option(
      cli, "highqualitybvh", params.highqualitybvh, "Use high quality BVH.");
  add_option(cli, "exposure", params.exposure, "Exposure value.");
  add_option(cli, "filmic", params.filmic, "Filmic tone mapping.");
  add_option(cli, "noparallel", params.noparallel, "Disable threading.");
}

// convert images
void run_render(const render_params& params_) {
  // copy params
  auto params = params_;

  // scene loading
  auto error = string{};
  print_progress_begin("load scene");
  auto old_scene = scene_data{};
  if (!load_scene(params.scene, old_scene, error)) print_fatal(error);
  print_progress_end();

  // add sky
  if (params.addsky) add_sky(old_scene);

  // add environment
  if (!params.envname.empty()) {
    print_progress_begin("add environment");
    add_environment(old_scene, params.envname);
    print_progress_end();
  }

  // camera
  params.camera = find_camera(old_scene, params.camname);

  // tesselation
  if (!old_scene.subdivs.empty()) {
    print_progress_begin("tesselate subdivs");
    tesselate_subdivs(old_scene);
    print_progress_end();
  }

  //
  //
  //
  //
  //

  auto data       = Data_Table{};
  auto scene_hash = create_scene_hash(old_scene, data);
  auto scene_view = create_scene_view(scene_hash);

  auto node        = scene_hash.cameras()[0];
  auto c           = camera_data{};
  c.aperture       = 999;
  auto scene_hash0 = scene_hash.edit(node, c);

  auto diff = make_diff(scene_hash.root, scene_hash0.root);
  apply_diff(scene_view, Scene_Hash(diff, data));

  // build bvh
  print_progress_begin("build bvh");
  auto bvh = make_bvh(scene_hash, params);
  print_progress_end();

  // init renderer
  print_progress_begin("build lights");
  auto lights = make_lights(scene_hash, params);
  print_progress_end();

  // fix renderer type if no lights
  if (lights.lights.empty() && is_sampler_lit(params)) {
    print_info("no lights presents, image will be black");
    params.sampler = trace_sampler_type::eyelight;
  }

  // state
  print_progress_begin("init state");
  auto state = make_state(scene_hash, params);
  print_progress_end();

  auto& scene = scene_hash;

  // render
  print_progress_begin("render image", params.samples);
  for (auto sample = 0; sample < params.samples; sample++) {
    trace_samples(state, scene, bvh, lights, params);
    if (params.savebatch && state.samples % params.batch == 0) {
      auto image = params.denoise ? get_denoised(state) : get_render(state);
      auto ext = "-s" + std::to_string(sample) + path_extension(params.output);
      auto outfilename = replace_extension(params.output, ext);
      if (!is_hdr_filename(params.output))
        image = tonemap_image(image, params.exposure, params.filmic);
      if (!save_image(outfilename, image, error)) print_fatal(error);
    }
    print_progress_next();
  }

  // save image
  print_progress_begin("save image");
  auto image = params.denoise ? get_denoised(state) : get_render(state);
  if (!is_hdr_filename(params.output))
    image = tonemap_image(image, params.exposure, params.filmic);
  if (!save_image(params.output, image, error)) print_fatal(error);
  print_progress_end();
}

// convert params
struct view_params : trace_params {
  string scene   = "scene.json";
  string output  = "out.png";
  string camname = "";
  bool   addsky  = false;
  string envname = "";
};

// Cli
void add_options(const cli_command& cli, view_params& params) {
  add_argument_with_config(
      cli, "scene", params.scene, "Scene filename.", "yscene.json");
  add_option(cli, "output", params.output, "Output filename.");
  add_option(cli, "camera", params.camname, "Camera name.");
  add_option(cli, "addsky", params.addsky, "Add sky.");
  add_option(cli, "envname", params.envname, "Add environment map.");
  add_option(
      cli, "resolution", params.resolution, "Image resolution.", {1, 4096});
  add_option(
      cli, "sampler", params.sampler, "Sampler type.", trace_sampler_names);
  add_option(cli, "falsecolor", params.falsecolor, "False color type.",
      trace_falsecolor_names);
  add_option(cli, "samples", params.samples, "Number of samples.", {1, 4096});
  add_option(cli, "bounces", params.bounces, "Number of bounces.", {1, 128});
  add_option(cli, "denoise", params.denoise, "Enable denoiser.");
  add_option(cli, "batch", params.batch, "Sample batch.");
  add_option(cli, "clamp", params.clamp, "Clamp params.", {10, flt_max});
  add_option(cli, "nocaustics", params.nocaustics, "Disable caustics.");
  add_option(cli, "envhidden", params.envhidden, "Hide environment.");
  add_option(cli, "tentfilter", params.tentfilter, "Filter image.");
  add_option(cli, "embreebvh", params.embreebvh, "Use Embree as BVH.");
  add_option(
      cli, "highqualitybvh", params.highqualitybvh, "Use high quality BVH.");
  add_option(cli, "exposure", params.exposure, "Exposure value.");
  add_option(cli, "filmic", params.filmic, "Filmic tone mapping.");
  add_option(cli, "noparallel", params.noparallel, "Disable threading.");
}

#ifndef YOCTO_OPENGL

// view scene
void run_view(const view_params& params) { print_fatal("Opengl not compiled"); }

#else

static void update_image_params(const glinput_state& input,
    const image_data& image, glimage_params& glparams) {
  glparams.window                           = input.window_size;
  glparams.framebuffer                      = input.framebuffer_viewport;
  std::tie(glparams.center, glparams.scale) = camera_imview(glparams.center,
      glparams.scale, {image.width, image.height}, glparams.window,
      glparams.fit);
}

static bool uiupdate_image_params(
    const glinput_state& input, glimage_params& glparams) {
  // handle mouse
  if (input.mouse_left && input.modifier_alt && !input.widgets_active) {
    if (input.modifier_ctrl) {
      glparams.scale *= yocto::pow(
          2.0f, (input.mouse_pos.y - input.mouse_last.y) * 0.001f);
      return true;
    } else {
      glparams.center += input.mouse_pos - input.mouse_last;
      return true;
    }
  }
  return false;
}

static bool uiupdate_camera_params(
    const glinput_state& input, camera_data& camera) {
  if (input.mouse_left && input.modifier_alt && !input.widgets_active) {
    auto dolly  = 0.0f;
    auto pan    = zero2f;
    auto rotate = zero2f;
    if (input.modifier_shift) {
      pan   = (input.mouse_pos - input.mouse_last) * camera.focus / 200.0f;
      pan.x = -pan.x;
    } else if (input.modifier_ctrl) {
      dolly = (input.mouse_pos.y - input.mouse_last.y) / 100.0f;
    } else {
      rotate = (input.mouse_pos - input.mouse_last) / 100.0f;
    }
    auto [frame, focus] = camera_turntable(
        camera.frame, camera.focus, rotate, dolly, pan);
    if (camera.frame != frame || camera.focus != focus) {
      camera.frame = frame;
      camera.focus = focus;
      return true;
    }
  }
  return false;
}

static bool draw_tonemap_params(
    const glinput_state& input, float& exposure, bool& filmic) {
  auto edited = 0;
  if (begin_glheader("tonemap")) {
    edited += draw_glslider("exposure", exposure, -5, 5);
    edited += draw_glcheckbox("filmic", filmic);
    end_glheader();
  }
  return (bool)edited;
}

static bool draw_image_inspector(const glinput_state& input,
    const image_data& image, const image_data& display,
    glimage_params& glparams) {
  if (begin_glheader("inspect")) {
    draw_glslider("zoom", glparams.scale, 0.1, 10);
    draw_glcheckbox("fit", glparams.fit);
    draw_glcoloredit("background", glparams.background);
    auto [i, j] = image_coords(input.mouse_pos, glparams.center, glparams.scale,
        {image.width, image.height});
    auto ij     = vec2i{i, j};
    draw_gldragger("mouse", ij);
    auto image_pixel   = zero4f;
    auto display_pixel = zero4f;
    if (i >= 0 && i < image.width && j >= 0 && j < image.height) {
      image_pixel   = image.pixels[j * image.width + i];
      display_pixel = image.pixels[j * image.width + i];
    }
    draw_glcoloredit("image", image_pixel);
    draw_glcoloredit("display", display_pixel);
    end_glheader();
  }
  return false;
}

struct scene_selection {
  int camera      = 0;
  int instance    = 0;
  int environment = 0;
  int shape       = 0;
  int texture     = 0;
  int material    = 0;
  int subdiv      = 0;
};

static bool draw_scene_editor(Scene_Hash& scene, scene_selection& selection,
    const function<void()>& before_edit) {
  return false;
#if 0
  auto edited = 0;
  if (begin_glheader("cameras")) {
    draw_glcombobox("camera", selection.camera, scene.camera_names);
    auto camera = scene.cameras.at(selection.camera);
    edited += draw_glcheckbox("ortho", camera.orthographic);
    edited += draw_glslider("lens", camera.lens, 0.001, 1);
    edited += draw_glslider("aspect", camera.aspect, 0.1, 5);
    edited += draw_glslider("film", camera.film, 0.1, 0.5);
    edited += draw_glslider("focus", camera.focus, 0.001, 100);
    edited += draw_glslider("aperture", camera.aperture, 0, 1);
    //   frame3f frame        = identity3x4f;
    if (edited) {
      if (before_edit) before_edit();
      scene.cameras.at(selection.camera) = camera;
    }
    end_glheader();
  }
  if (begin_glheader("environments")) {
    draw_glcombobox(
        "environment", selection.environment, scene.environment_names);
    auto environment = scene.environments.at(selection.environment);
    edited += draw_glcoloredithdr("emission", environment.emission);
    edited += draw_glcombobox(
        "emission_tex", environment.emission_tex, scene.texture_names, true);
    //   frame3f frame        = identity3x4f;
    if (edited) {
      if (before_edit) before_edit();
      scene.environments.at(selection.environment) = environment;
    }
    end_glheader();
  }
  if (begin_glheader("instances")) {
    draw_glcombobox("instance", selection.instance, scene.instance_names);
    auto instance = scene.instances.at(selection.instance);
    edited += draw_glcombobox("shape", instance.shape, scene.shape_names);
    edited += draw_glcombobox(
        "material", instance.material, scene.material_names);
    //   frame3f frame        = identity3x4f;
    if (edited) {
      if (before_edit) before_edit();
      scene.instances.at(selection.instance) = instance;
    }
    end_glheader();
  }
  if (begin_glheader("materials")) {
    draw_glcombobox("material", selection.material, scene.material_names);
    auto material = scene.materials.at(selection.material);
    edited += draw_glcoloredithdr("emission", material.emission);
    edited += draw_glcombobox(
        "emission_tex", material.emission_tex, scene.texture_names, true);
    edited += draw_glcoloredithdr("color", material.color);
    edited += draw_glcombobox(
        "color_tex", material.color_tex, scene.texture_names, true);
    edited += draw_glslider("roughness", material.roughness, 0, 1);
    edited += draw_glcombobox(
        "roughness_tex", material.roughness_tex, scene.texture_names, true);
    edited += draw_glslider("metallic", material.metallic, 0, 1);
    edited += draw_glslider("ior", material.ior, 0.1, 5);
    if (edited) {
      if (before_edit) before_edit();
      scene.materials.at(selection.material) = material;
    }
    end_glheader();
  }
  if (begin_glheader("shapes")) {
    draw_glcombobox("shape", selection.shape, scene.shape_names);
    auto& shape = scene.shapes.at(selection.shape);
    draw_gllabel("points", (int)shape.points.size());
    draw_gllabel("lines", (int)shape.lines.size());
    draw_gllabel("triangles", (int)shape.triangles.size());
    draw_gllabel("quads", (int)shape.quads.size());
    draw_gllabel("positions", (int)shape.positions.size());
    draw_gllabel("normals", (int)shape.normals.size());
    draw_gllabel("texcoords", (int)shape.texcoords.size());
    draw_gllabel("colors", (int)shape.colors.size());
    draw_gllabel("radius", (int)shape.radius.size());
    draw_gllabel("tangents", (int)shape.tangents.size());
    end_glheader();
  }
  if (begin_glheader("textures")) {
    draw_glcombobox("texture", selection.texture, scene.texture_names);
    auto& texture = scene.textures.at(selection.texture);
    draw_gllabel("width", texture.width);
    draw_gllabel("height", texture.height);
    draw_gllabel("linear", texture.linear);
    draw_gllabel("byte", !texture.pixelsb.empty());
    end_glheader();
  }
  if (begin_glheader("subdivs")) {
    draw_glcombobox("subdiv", selection.subdiv, scene.subdiv_names);
    auto& subdiv = scene.subdivs.at(selection.subdiv);
    draw_gllabel("quadspos", (int)subdiv.quadspos.size());
    draw_gllabel("quadsnorm", (int)subdiv.quadsnorm.size());
    draw_gllabel("quadstexcoord", (int)subdiv.quadstexcoord.size());
    draw_gllabel("positions", (int)subdiv.positions.size());
    draw_gllabel("normals", (int)subdiv.normals.size());
    draw_gllabel("texcoords", (int)subdiv.texcoords.size());
    end_glheader();
  }
  return (bool)edited;
#endif
}

void view_scene(const string& title, const string& name, Scene_Hash& scene,
    const trace_params& params_, bool print, bool edit) {
  // copy params and camera
  auto params = params_;

  // build bvh
  if (print) print_progress_begin("build bvh");
  auto bvh = make_bvh(scene, params);
  if (print) print_progress_end();

  // init renderer
  if (print) print_progress_begin("init lights");
  auto lights = make_lights(scene, params);
  if (print) print_progress_end();

  // fix renderer type if no lights
  if (lights.lights.empty() && is_sampler_lit(params)) {
    if (print) print_info("no lights presents --- switching to eyelight");
    params.sampler = trace_sampler_type::eyelight;
  }

  // init state
  if (print) print_progress_begin("init state");
  auto state   = make_state(scene, params);
  auto image   = make_image(state.width, state.height, true);
  auto display = make_image(state.width, state.height, false);
  auto render  = make_image(state.width, state.height, true);
  if (print) print_progress_end();

  // opengl image
  auto glimage  = glimage_state{};
  auto glparams = glimage_params{};

  // top level combo
  auto names    = vector<string>{name};
  auto selected = 0;

  // camera names
  auto camera_names = vector<string>{};
  if (camera_names.empty()) {
    for (auto idx = 0; idx < (int)scene.cameras().size(); idx++) {
      camera_names.push_back("camera" + std::to_string(idx + 1));
    }
  }

  // renderer update
  auto render_update  = std::atomic<bool>{};
  auto render_current = std::atomic<int>{};
  auto render_mutex   = std::mutex{};
  auto render_worker  = future<void>{};
  auto render_stop    = atomic<bool>{};
  //  auto atomic_scene   = atomic<Hash_Node*>{scene.root};
  auto reset_display = [&]() {
    // stop render
    render_stop = true;
    if (render_worker.valid()) render_worker.get();

    state   = make_state(scene, params);
    image   = make_image(state.width, state.height, true);
    display = make_image(state.width, state.height, false);
    render  = make_image(state.width, state.height, true);

    render_worker = {};
    render_stop   = false;

    // preview
    auto pparams = params;
    pparams.resolution /= params.pratio;
    pparams.samples = 1;
    auto pstate     = make_state(scene, pparams);
    trace_samples(pstate, scene, bvh, lights, pparams);
    auto preview = get_render(pstate);
    for (auto idx = 0; idx < state.width * state.height; idx++) {
      auto i = idx % render.width, j = idx / render.width;
      auto pi            = clamp(i / params.pratio, 0, preview.width - 1),
           pj            = clamp(j / params.pratio, 0, preview.height - 1);
      render.pixels[idx] = preview.pixels[pj * preview.width + pi];
    }
    // if (current > 0) return;
    {
      auto lock      = std::lock_guard{render_mutex};
      render_current = 0;
      image          = render;
      tonemap_image_mt(display, image, params.exposure, params.filmic);
      render_update = true;
    }

    // start renderer
    render_worker = std::async(std::launch::async, [&]() {
      for (auto sample = 0; sample < params.samples; sample += params.batch) {
        if (render_stop) return;
        auto  r     = scene.root;
        auto& d     = scene.data;
        auto  scene = Scene_Hash{r, d};
        parallel_for(state.width, state.height, [&](int i, int j) {
          for (auto s = 0; s < params.batch; s++) {
            if (render_stop) return;
            trace_sample(state, scene, bvh, lights, i, j, params);
          }
        });
        state.samples += params.batch;
        if (!render_stop) {
          auto lock      = std::lock_guard{render_mutex};
          render_current = state.samples;
          if (!params.denoise || render_stop) {
            get_render(render, state);
          } else {
            get_denoised(render, state);
          }
          image = render;
          tonemap_image_mt(display, image, params.exposure, params.filmic);
          render_update = true;
        }
      }
    });
  };

  // stop render
  auto stop_render = [&]() {
    render_stop = true;
    if (render_worker.valid()) render_worker.get();
  };

  // start rendering
  reset_display();

  // prepare selection
  auto selection = scene_selection{};

  // callbacks
  auto callbacks    = glwindow_callbacks{};
  callbacks.init_cb = [&](const glinput_state& input) {
    auto lock = std::lock_guard{render_mutex};
    init_image(glimage);
    set_image(glimage, display);
  };
  callbacks.clear_cb = [&](const glinput_state& input) {
    clear_image(glimage);
  };
  callbacks.draw_cb = [&](const glinput_state& input) {
    // update image
    if (render_update) {
      auto lock = std::lock_guard{render_mutex};
      set_image(glimage, display);
      render_update = false;
    }
    update_image_params(input, image, glparams);
    draw_image(glimage, glparams);
  };
  callbacks.widgets_cb = [&](const glinput_state& input) {
    auto edited = 0;
    draw_glcombobox("name", selected, names);
    auto current = (int)render_current;
    draw_glprogressbar("sample", current, params.samples);
    if (begin_glheader("render")) {
      auto edited  = 0;
      auto tparams = params;
      edited += draw_glcombobox("camera", tparams.camera, camera_names);
      edited += draw_glslider("resolution", tparams.resolution, 180, 4096);
      edited += draw_glslider("samples", tparams.samples, 16, 4096);
      edited += draw_glcombobox(
          "tracer", (int&)tparams.sampler, trace_sampler_names);
      edited += draw_glcombobox(
          "false color", (int&)tparams.falsecolor, trace_falsecolor_names);
      edited += draw_glslider("bounces", tparams.bounces, 1, 128);
      edited += draw_glslider("batch", tparams.batch, 1, 16);
      edited += draw_glslider("clamp", tparams.clamp, 10, 1000);
      edited += draw_glcheckbox("envhidden", tparams.envhidden);
      continue_glline();
      edited += draw_glcheckbox("filter", tparams.tentfilter);
      edited += draw_glslider("pratio", tparams.pratio, 1, 64);
      // edited += draw_glslider("exposure", tparams.exposure, -5, 5);
      end_glheader();
      if (edited) {
        stop_render();
        params = tparams;
        reset_display();
      }
    }
    if (begin_glheader("tonemap")) {
      edited += draw_glslider("exposure", params.exposure, -5, 5);
      edited += draw_glcheckbox("filmic", params.filmic);
      edited += draw_glcheckbox("denoise", params.denoise);
      end_glheader();
      if (edited) {
        tonemap_image_mt(display, image, params.exposure, params.filmic);
        set_image(glimage, display);
      }
    }
    draw_image_inspector(input, image, display, glparams);
    if (edit) {
      if (draw_scene_editor(scene, selection, [&]() { stop_render(); })) {
        reset_display();
      }
    }
  };

  callbacks.uiupdate_cb = [&](const glinput_state& input) {
    auto camera = scene.cameras(params.camera);
    if (uiupdate_camera_params(input, camera)) {
      // stop_render();
      scene.root = edit_node(
          scene.cameras()[params.camera], camera, scene.data);
      // atomic_scene = scene.root;
      // reset_display();
    }
  };

  // run ui
  run_ui({1280 + 320, 720}, title, callbacks);

  // done
  stop_render();
}

// view scene
void run_view(const view_params& params_) {
  // copy params
  auto params = params_;

  // load scene
  auto error = string{};
  print_progress_begin("load scene");
  auto scene = scene_data{};
  if (!load_scene(params.scene, scene, error)) print_fatal(error);
  print_progress_end();

  // add sky
  if (params.addsky) add_sky(scene);

  // add environment
  if (!params.envname.empty()) {
    print_progress_begin("add environment");
    add_environment(scene, params.envname);
    print_progress_end();
  }

  // tesselation
  if (!scene.subdivs.empty()) {
    print_progress_begin("tesselate subdivs");
    tesselate_subdivs(scene);
    print_progress_end();
  }

  // find camera
  params.camera = find_camera(scene, params.camname);

  // run view
  auto data   = Data_Table{};
  auto gscene = create_scene_hash(scene, data);
  view_scene("yscene", params.scene, gscene, params, false, true);
}

#endif

struct glview_params {
  string scene   = "scene.json"s;
  string camname = "";
};

// Cli
void add_options(const cli_command& cli, glview_params& params) {
  add_argument(cli, "scene", params.scene, "Input scene.");
  add_option(cli, "camera", params.camname, "Camera name.");
}

#ifndef YOCTO_OPENGL

// view scene
void run_glview(const glview_params& params) {
  print_fatal("Opengl not compiled");
}

#else

void run_glview(const glview_params& params_) {
  // copy params
  auto params = params_;

  // loading scene
  auto error = string{};
  print_progress_begin("load scene");
  auto scene = scene_data{};
  if (!load_scene(params.scene, scene, error)) print_fatal(error);
  print_progress_end();

  // tesselation
  if (!scene.subdivs.empty()) {
    print_progress_begin("tesselate subdivs");
    tesselate_subdivs(scene);
    print_progress_end();
  }

  // camera
  auto glparams   = glscene_params{};
  glparams.camera = find_camera(scene, params.camname);

  // run viewer
  glview_scene("yscene", params.scene, scene, glparams);
}

#endif

struct app_params {
  string         command = "convert";
  convert_params convert = {};
  info_params    info    = {};
  render_params  render  = {};
  view_params    view    = {};
  glview_params  glview  = {};
};

// Cli
void add_options(const cli_command& cli, app_params& params) {
  set_command_var(cli, params.command);
  add_command(cli, "convert", params.convert, "Convert scenes.");
  add_command(cli, "info", params.info, "Print scenes info.");
  add_command(cli, "render", params.render, "Render scenes.");
  add_command(cli, "view", params.view, "View scenes.");
  add_command(cli, "glview", params.glview, "View scenes with OpenGL.");
}

// Run
void run(const vector<string>& args) {
  // command line parameters
  auto error  = string{};
  auto params = app_params{};
  auto cli    = make_cli("yscene", params, "Process and view scenes.");
  if (!parse_cli(cli, args, error)) print_fatal(error);

  // dispatch commands
  if (params.command == "convert") {
    return run_convert(params.convert);
  } else if (params.command == "info") {
    return run_info(params.info);
  } else if (params.command == "render") {
    return run_render(params.render);
  } else if (params.command == "view") {
    return run_view(params.view);
  } else if (params.command == "glview") {
    return run_glview(params.glview);
  } else {
    print_fatal("yscene; unknown command");
  }
}

// Main
int main(int argc, const char* argv[]) { run(make_cli_args(argc, argv)); }
