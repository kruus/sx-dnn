/*******************************************************************************
* Copyright 2016-2020 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef COMMON_BATCH_NORMALIZATION_PD_HPP
#define COMMON_BATCH_NORMALIZATION_PD_HPP

#include "dnnl.h"

#include "c_types_map.hpp"
#include "primitive_desc.hpp"
#include "utils.hpp"

namespace dnnl {
namespace impl {

struct batch_normalization_fwd_pd_t;

#define BNORM_CONSTRUCTOR_DEBUG 0
#if BNORM_CONSTRUCTOR_DEBUG
inline void dump(memory_desc_t const* pmd){
    if(pmd == nullptr){
        printf(" &md=NULL");
        return;
    }
    printf(" dump md @%p",(void*)pmd);
    memory_desc_t const& md = *pmd;
        
    typedef long unsigned lu;
    printf(" md{%lu,{",(lu)md.ndims);
    if(md.ndims) {
        for(int i=0;i<md.ndims;++i) {
            printf("%lu%c",(lu)md.dims[i],(i==md.ndims-1? '}':','));
        }
    }else printf("}");
    printf(",dt=%d,padded_dims={",(int)md.data_type);
    if(md.ndims) {
        for(int i=0;i<md.ndims;++i) {
            printf("%lu%c",(lu)md.padded_dims[i],(i==md.ndims-1? '}':','));
        }
    }else printf("}");
    printf(",padded_offsets={");
    if(md.ndims) {
        for(int i=0;i<md.ndims;++i) {
            printf("%ld%c",(long)md.padded_offsets[i],(i==md.ndims-1? '}':','));
        }
    }else printf("}");
    printf(",offset0=%ld,format_kind=%d",(long)md.offset0,(int)md.format_kind);
    printf("}");
}
#endif


struct batch_normalization_pd_t : public primitive_desc_t {
    static constexpr auto base_pkind = primitive_kind::batch_normalization;

#if BNORM_CONSTRUCTOR_DEBUG
    void constructor_debug();
    void init_ws_debug(long unsigned bytes, long unsigned elements);
    memory_desc_t const& ws_md() const { return this->ws_md_; }
#endif
    batch_normalization_pd_t(const batch_normalization_desc_t *adesc,
            const primitive_attr_t *attr,
            const batch_normalization_fwd_pd_t *hint_fwd_pd)
        : primitive_desc_t(attr, base_pkind)
        , desc_(*adesc)
        , hint_fwd_pd_(hint_fwd_pd)
        , data_md_(desc_.data_desc)
        , stat_md_(desc_.stat_desc)
        , scaleshift_md_(desc_.data_scaleshift_desc)
        , ws_md_() {
#if BNORM_CONSTRUCTOR_DEBUG
            constructor_debug();
#endif
        }

    const batch_normalization_desc_t *desc() const { return &desc_; }
    virtual const op_desc_t *op_desc() const override {
        return reinterpret_cast<const op_desc_t *>(this->desc());
    }

    virtual status_t query(query_t what, int idx, void *result) const override {
        switch (what) {
            case query::prop_kind:
                *(prop_kind_t *)result = desc()->prop_kind;
                break;
            case query::batch_normalization_d:
                *(const batch_normalization_desc_t **)result = desc();
                break;
            default: return primitive_desc_t::query(what, idx, result);
        }
        return status::success;
    }

    /* common batch_normalization aux functions */

    dim_t MB() const { return data_desc().dims[0]; }
    dim_t C() const { return data_desc().dims[1]; }
    dim_t D() const { return ndims() >= 5 ? data_desc().dims[ndims() - 3] : 1; }
    dim_t H() const { return ndims() >= 4 ? data_desc().dims[ndims() - 2] : 1; }
    dim_t W() const { return ndims() >= 3 ? data_desc().dims[ndims() - 1] : 1; }

    int ndims() const { return desc_.data_desc.ndims; }

    bool stats_is_src() const { return desc_.flags & dnnl_use_global_stats; }
    bool use_scaleshift() const { return desc_.flags & dnnl_use_scaleshift; }
    bool use_global_stats() const {
        return desc_.flags & dnnl_use_global_stats;
    }
    bool fuse_norm_relu() const { return desc_.flags & dnnl_fuse_norm_relu; }
    bool with_relu_post_op() const {
        const auto &p = this->attr()->post_ops_;
        return p.len_ == 1 && p.entry_[0].is_relu(true, true);
    }

    bool is_fwd() const {
        return utils::one_of(desc_.prop_kind, prop_kind::forward_training,
                prop_kind::forward_inference);
    }
    bool is_bwd() const { return !this->is_fwd(); }
    bool is_training() const {
        return desc_.prop_kind == prop_kind::forward_training;
    }

    bool has_zero_dim_memory() const {
        return memory_desc_wrapper(desc_.data_desc).has_zero_dim();
    }

protected:
    batch_normalization_desc_t desc_;
    const batch_normalization_fwd_pd_t *hint_fwd_pd_;

    memory_desc_t data_md_;
    memory_desc_t stat_md_;
    memory_desc_t scaleshift_md_;

    memory_desc_t ws_md_;

    virtual void init_default_ws(size_t bits_per_element) {
        const auto data_mdw = memory_desc_wrapper(data_md_);

        const dim_t data_nelems = data_mdw.nelems(true);
        const dim_t bits_per_byte = 8;
        const dims_t ws_sz = {(dim_t)utils::div_up(
                data_nelems * bits_per_element, bits_per_byte)};
#if BNORM_CONSTRUCTOR_DEBUG
        printf(" ws_sz[1]=%lu", ws_sz[1]);
#endif
        dnnl_memory_desc_init_by_tag(
                &ws_md_, 1, ws_sz, impl::data_type::u8, format_tag::x);
#if BNORM_CONSTRUCTOR_DEBUG
        printf(" ws_sz[1]=%lu", ws_sz[1]);
        init_ws_debug(ws_sz[0], data_nelems);
#endif // BNORM_CONSTRUCTOR_DEBUG
    }

private:
    const memory_desc_t &data_desc() const { return desc_.data_desc; }
};


struct batch_normalization_fwd_pd_t : public batch_normalization_pd_t {
    typedef batch_normalization_fwd_pd_t base_class;
    typedef batch_normalization_fwd_pd_t hint_class;

    batch_normalization_fwd_pd_t(const batch_normalization_desc_t *adesc,
            const primitive_attr_t *attr,
            const batch_normalization_fwd_pd_t *hint_fwd_pd)
        : batch_normalization_pd_t(adesc, attr, hint_fwd_pd) {}

    virtual arg_usage_t arg_usage(int arg) const override {
        if (arg == DNNL_ARG_SRC) return arg_usage_t::input;
        if (arg == DNNL_ARG_DST) return arg_usage_t::output;

        if (utils::one_of(arg, DNNL_ARG_MEAN, DNNL_ARG_VARIANCE)) {
            if (stats_is_src()) return arg_usage_t::input;
            if (!stats_is_src() && is_training()) return arg_usage_t::output;
            return arg_usage_t::unused;
        }

        if (arg == DNNL_ARG_SCALE_SHIFT && use_scaleshift())
            return arg_usage_t::input;

        if (arg == DNNL_ARG_WORKSPACE && is_training() && fuse_norm_relu())
            return arg_usage_t::output;

        return primitive_desc_t::arg_usage(arg);
    }

    virtual const memory_desc_t *arg_md(int arg) const override {
        switch (arg) {
            case DNNL_ARG_SRC: return src_md(0);
            case DNNL_ARG_DST: return dst_md(0);
            case DNNL_ARG_MEAN: return stats_is_src() ? src_md(1) : dst_md(1);
            case DNNL_ARG_VARIANCE:
                return stats_is_src() ? src_md(2) : dst_md(2);
            case DNNL_ARG_SCALE_SHIFT: return weights_md(0);
            default: return batch_normalization_pd_t::arg_md(arg);
        }
    }

    virtual const memory_desc_t *src_md(int index = 0) const override {
        if (index == 0) return &data_md_;
        if (stats_is_src() && (index == 1 || index == 2)) return &stat_md_;
        return &glob_zero_md;
    }

    virtual const memory_desc_t *dst_md(int index = 0) const override {
        if (index == 0) return &data_md_;
        if (!stats_is_src() && is_training() && (index == 1 || index == 2))
            return &stat_md_;
        return &glob_zero_md;
    }

    virtual const memory_desc_t *weights_md(int index = 0) const override {
        return index == 0 ? &scaleshift_md_ : &glob_zero_md;
    }

    virtual const memory_desc_t *workspace_md(int index = 0) const override {
#if BNORM_CONSTRUCTOR_DEBUG
        printf(" bnorm worspace_md(%d),training=%d,fuse_norm_relu=%d\n",
               index, (int)is_training(), (int)fuse_norm_relu());
#endif
        return index == 0 && is_training() && fuse_norm_relu() ? &ws_md_
                                                               : &glob_zero_md;
    }

    const memory_desc_t *stat_md() const {
        return stats_is_src() ? src_md(1) : dst_md(1);
    }

    virtual int n_inputs() const override {
        return 1 + 2 * stats_is_src() + use_scaleshift();
    }
    virtual int n_outputs() const override {
        return 1 + (fuse_norm_relu() + 2 * (!stats_is_src())) * is_training();
    }
};


struct batch_normalization_bwd_pd_t : public batch_normalization_pd_t {
    typedef batch_normalization_bwd_pd_t base_class;
    typedef batch_normalization_fwd_pd_t hint_class;

    batch_normalization_bwd_pd_t(const batch_normalization_desc_t *adesc,
            const primitive_attr_t *attr,
            const batch_normalization_fwd_pd_t *hint_fwd_pd)
        : batch_normalization_pd_t(adesc, attr, hint_fwd_pd)
        , diff_data_md_(desc_.diff_data_desc)
        , diff_scaleshift_md_(desc_.diff_data_scaleshift_desc) {}

    virtual arg_usage_t arg_usage(int arg) const override {
        if (utils::one_of(arg, DNNL_ARG_SRC, DNNL_ARG_MEAN, DNNL_ARG_VARIANCE,
                    DNNL_ARG_DIFF_DST))
            return arg_usage_t::input;

        if (arg == DNNL_ARG_SCALE_SHIFT && use_scaleshift())
            return arg_usage_t::input;

        if (arg == DNNL_ARG_WORKSPACE && fuse_norm_relu())
            return arg_usage_t::input;

        if (arg == DNNL_ARG_DIFF_SRC) return arg_usage_t::output;

        if (arg == DNNL_ARG_DIFF_SCALE_SHIFT && use_scaleshift())
            return arg_usage_t::output;

        return primitive_desc_t::arg_usage(arg);
    }

    virtual const memory_desc_t *arg_md(int arg) const override {
        switch (arg) {
            case DNNL_ARG_SRC: return src_md(0);
            case DNNL_ARG_MEAN: return src_md(1);
            case DNNL_ARG_VARIANCE: return src_md(2);
            case DNNL_ARG_SCALE_SHIFT: return weights_md(0);
            case DNNL_ARG_DIFF_SRC: return diff_src_md(0);
            case DNNL_ARG_DIFF_DST: return diff_dst_md(0);
            case DNNL_ARG_DIFF_SCALE_SHIFT: return diff_weights_md(0);
            default: return batch_normalization_pd_t::arg_md(arg);
        }
    }

    virtual const memory_desc_t *src_md(int index = 0) const override {
        return index == 0 ? &data_md_ : index <= 2 ? &stat_md_ : &glob_zero_md;
    }
    virtual const memory_desc_t *diff_dst_md(int index = 0) const override {
        return index == 0 ? &diff_data_md_ : &glob_zero_md;
    }
    virtual const memory_desc_t *diff_src_md(int index = 0) const override {
        return index == 0 ? &diff_data_md_ : &glob_zero_md;
    }

    virtual const memory_desc_t *weights_md(int index = 0) const override {
        return index == 0 ? &scaleshift_md_ : &glob_zero_md;
    }
    virtual const memory_desc_t *diff_weights_md(int index = 0) const override {
        return index == 0 ? &diff_scaleshift_md_ : &glob_zero_md;
    }

    virtual const memory_desc_t *workspace_md(int index = 0) const override {
        return index == 0 && fuse_norm_relu() ? &ws_md_ : &glob_zero_md;
    }

    const memory_desc_t *stat_md() const { return src_md(1); }

    virtual int n_inputs() const override {
        return 4 + use_scaleshift() + fuse_norm_relu();
    }
    virtual int n_outputs() const override {
        return 1 + (!types::is_zero_md(diff_weights_md()));
    }

protected:
    memory_desc_t diff_data_md_;
    memory_desc_t diff_scaleshift_md_;

    bool set_default_formats_common() {
        if (diff_data_md_.format_kind != format_kind::any) return true;

        return memory_desc_init_by_md_and_dt(
                       diff_data_md_, data_md_, diff_data_md_.data_type)
                == status::success;
    }
};

#if BNORM_CONSTRUCTOR_DEBUG
inline void batch_normalization_pd_t::constructor_debug() {
    printf(" +bnorm_pd(hint_fwd_pd_=%p)", (void*)hint_fwd_pd_);
    fflush(stdout);
    if(hint_fwd_pd_ != nullptr){
        printf(" hint_fwd_pd_->ws_md_ @%p",(void*)&hint_fwd_pd_->ws_md_);
        char s[80]; dnnl_md2fmt_str(s, 80, &hint_fwd_pd_->ws_md_);
        printf(" is %s",s); fflush(stdout);
        printf("\n\thint "); fflush(stdout);
        dump(&hint_fwd_pd_->ws_md_);
    }
    printf("\n");
}

inline void batch_normalization_pd_t::init_ws_debug(long unsigned bytes, long unsigned elements) {
    char s[80]; dnnl_md2fmt_str(s, 80, &ws_md_);
    printf(" bnorm-pd-init_default_ws[%lu bytes] for %lu elements @%p %s",
           bytes, elements, (void*)&ws_md_, s);
    printf("\n\tinit ws ");
    dump(&ws_md_);
    printf("\n\thint ws ");
    if (hint_fwd_pd_) dump(&hint_fwd_pd_->ws_md_);
    else printf(" no hint");
    printf("\n");
}
#endif // BNORM_CONSTRUCTOR_DEBUG

} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s
