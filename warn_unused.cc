// ORI: Downloaded from https://raw.githubusercontent.com/rofirrim/gcc-plugins/master/blog_03/warn_unused.cc
// ORI: For this to compile, you need to install the package `gcc-4.8-plugin-dev`
// ORI: You also need the switch -I/usr/lib/gcc/x86_64-linux-gnu/4.8/plugin/include

// ORI: To test this, run the following lines
//      g++ warn_unused.cc  -I/usr/lib/gcc/x86_64-linux-gnu/4.8/plugin/include -std=gnu++11 -Wno-literal-suffix -shared -o warn_unused.so -fPIC -o warn_unused.so
//      g++ test.cc -fplugin=./warn_unused.so  -Wall -std=c++11

#include <iostream>
#include <cassert>
#include <set>
#include <utility>
#include <algorithm>

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include "plugin-version.h"

#include "cp/cp-tree.h"
#include "tree-pass.h"
#include "basic-block.h"
#include "function.h"
#include "gimple.h"
#include "plugin.h"
#if __GNUC__ >= 5
#include "context.h"
#include "internal-fn.h"
#include "predict.h"
#include "tree.h"
#include "tree-ssa-alias.h"
#include "gimple-expr.h"
#include "gimple-ssa.h"
#include "tree-pretty-print.h"
#include "tree-ssa-operands.h"
#include "tree-phinodes.h"
#include "gimple-pretty-print.h"
#include "gimple-iterator.h"
#include "gimple-walk.h"
#include "diagnostic.h"
#include "stringpool.h"

#include "ssa-iterators.h"
#endif


// We must assert that this plugin is GPL compatible
int plugin_is_GPL_compatible;

static struct plugin_info my_gcc_plugin_info = { "1.0", "This plugin emits warn_unused_result for C++" };

static struct attribute_spec my_attr = 
  {"warn_unused_result_type", 0, 0, false, true, false, nullptr, false};

static void warn_unused_result_lhs(const std::set<tree>& unused_lhs, function *fun)
{
  basic_block bb;
  FOR_ALL_BB_FN(bb, fun)
  {
    gimple_stmt_iterator gsi;
    for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gimple stmt = gsi_stmt (gsi);

      switch (gimple_code(stmt))
      {
        case GIMPLE_CALL:
          {
            tree lhs = gimple_call_lhs(stmt);
            if (unused_lhs.find(lhs) != unused_lhs.end())
            {
              // Deliberately similar to the code in tree-cfg.c
              tree fdecl = gimple_call_fndecl (stmt);
              tree ftype = gimple_call_fntype (stmt);

              if (lookup_attribute ("warn_unused_result", TYPE_ATTRIBUTES (ftype)))
              {
                location_t loc = gimple_location (stmt);

                if (fdecl)
                  warning_at (loc, OPT_Wunused_result,
                      "ignoring return value of %qD, "
                      "declared with attribute warn_unused_result",
                      fdecl);
                else
                  warning_at (loc, OPT_Wunused_result,
                      "ignoring return value of function "
                      "declared with attribute warn_unused_result");
              }
            }
            break;
          }
        default:
          // Do nothing
          break;
      }
    }
  }
}


static void erase_if_used_lhs(std::set<tree>& potential_unused_lhs,
    tree t)
{
  if (t == NULL)
    return;

  switch (TREE_CODE(t))
  {
    case VAR_DECL:
      if (DECL_ARTIFICIAL(t)
          && potential_unused_lhs.find(t) != potential_unused_lhs.end())
      {
        potential_unused_lhs.erase(t);
      }
      break;
    case COMPONENT_REF:
      erase_if_used_lhs(potential_unused_lhs, TREE_OPERAND(t, 0));
      break;
    default:
      {
        // TODO: are there more cases?
        break;
      }
  }
}

static std::set<tree> gather_unused_lhs(function* fun)
{
  std::set<tree> potential_unused_lhs;
  
  basic_block bb;
  FOR_ALL_BB_FN(bb, fun)
  {
    gimple_stmt_iterator gsi;
    for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gimple stmt = gsi_stmt (gsi);
      
      switch (gimple_code(stmt))
      {
      case GIMPLE_CALL:
      {
        tree lhs = gimple_call_lhs(stmt);
        if (lhs && TREE_CODE(lhs) == VAR_DECL && DECL_ARTIFICIAL(lhs))
          potential_unused_lhs.insert(lhs);
        
        unsigned nargs = gimple_call_num_args(stmt);
        for (unsigned i = 0; i < nargs; i++)
        {
          tree arg = gimple_call_arg(stmt, i);
          erase_if_used_lhs(potential_unused_lhs, arg);
        }
        break;

      }
      case GIMPLE_ASSIGN:
      {
        if (gimple_clobber_p(stmt)) 
          continue;
        
        tree lhs = gimple_assign_lhs(stmt);
        erase_if_used_lhs(potential_unused_lhs, lhs);
        
        tree rhs1 = gimple_assign_rhs1(stmt);
        erase_if_used_lhs(potential_unused_lhs, rhs1);
        
        tree rhs2 = gimple_assign_rhs2(stmt);
        if (rhs2 != NULL)
          erase_if_used_lhs(potential_unused_lhs, rhs2);
        
        tree rhs3 = gimple_assign_rhs3(stmt);
        if (rhs3 != NULL)
          erase_if_used_lhs(potential_unused_lhs, rhs3);
        
        break;
      }
      break;
      case GIMPLE_RETURN:
      {
        erase_if_used_lhs(potential_unused_lhs,
                          gimple_return_retval(stmt));
        break;
      }
      default:
        // do nothing
        break;
      }
    }
  }

  std::set<tree> unused_lhs( potential_unused_lhs);
  return unused_lhs;
}

#if __GNUC__ >= 4
static struct gimple_opt_pass myplugin_pass;
int dummy_124397ydk = ([] {
    myplugin_pass.pass.type = GIMPLE_PASS;
    myplugin_pass.pass.name = "warn_unused";
    myplugin_pass.pass.gate = [] { return true; };
    myplugin_pass.pass.execute = [] { 
      std::set<tree> unused_lhs = gather_unused_lhs(cfun);
      warn_unused_result_lhs(unused_lhs, cfun);
      return 0U;
    };
    return 0;
  })();

#endif

#if __GNUC__ >= 5
namespace
{
const pass_data warn_unused_result_cxx_data = 
{
  GIMPLE_PASS,
  "warn_unused_result_cxx", /* name */
  OPTGROUP_NONE,             /* optinfo_flags */
  TV_NONE,                   /* tv_id */
  PROP_gimple_any,           /* properties_required */
  0,                         /* properties_provided */
  0,                         /* properties_destroyed */
  0,                         /* todo_flags_start */
  0                          /* todo_flags_finish */
};

struct warn_unused_result_cxx : gimple_opt_pass
{
  warn_unused_result_cxx(gcc::context *ctx)
    : gimple_opt_pass(warn_unused_result_cxx_data, ctx)
  {
  }

  virtual unsigned int execute(function *fun) override
  {
    // This phase has two steps, first we remove redundant LHS from GIMPLE_CALLs
    std::set<tree> unused_lhs = gather_unused_lhs(fun);
    warn_unused_result_lhs(unused_lhs, fun);

    return 0;
  }

  virtual warn_unused_result_cxx* clone() override
  {
    // We do not clone ourselves
    return this;
  }
};
}
#endif


int plugin_init (struct plugin_name_args *plugin_info,
    struct plugin_gcc_version *version)
{
  // We check the current gcc loading this plugin against the gcc we used to
  // created this plugin
  if (!plugin_default_version_check (version, &gcc_version))
  {
    std::cerr << "This GCC plugin is for version " << GCCPLUGIN_VERSION_MAJOR << "." << GCCPLUGIN_VERSION_MINOR << "\n";
    return 1;
  }

  register_callback(plugin_info->base_name,
      /* event */ PLUGIN_INFO,
      /* callback */ NULL, /* user_data */ &my_gcc_plugin_info);

  // Register the phase right after cfg
  struct register_pass_info pass_info;

#if __GNUC__ >= 4
  pass_info.pass = &myplugin_pass.pass;
#else
  pass_info.pass = new warn_unused_result_cxx(g);
#endif
  pass_info.reference_pass_name = "cfg";
  pass_info.ref_pass_instance_number = 1;
  pass_info.pos_op = PASS_POS_INSERT_AFTER;

  register_callback (plugin_info->base_name, PLUGIN_ATTRIBUTES, 
                     [](void *, void *) { register_attribute(&my_attr); }, nullptr);
  register_callback (plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, nullptr, &pass_info);

  return 0;
}

