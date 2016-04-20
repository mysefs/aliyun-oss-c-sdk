#include "CuTest.h"
#include "test.h"
#include "aos_log.h"
#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "aos_transport.h"
#include "aos_http_io.h"
#include "oss_auth.h"
#include "oss_util.h"
#include "oss_xml.h"
#include "oss_api.h"
#include "oss_config.h"
#include "oss_test_util.h"
#include "apr_time.h"

void test_live_setup(CuTest *tc)
{
    aos_pool_t *p = NULL;
    int is_cname = 0;
    aos_status_t *s = NULL;
    oss_request_options_t *options = NULL;
    oss_acl_e oss_acl = OSS_ACL_PRIVATE;

    //create test bucket
    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, is_cname);
    s = create_test_bucket(options, TEST_BUCKET_NAME, oss_acl);

    CuAssertIntEquals(tc, 200, s->code);

    aos_pool_destroy(p);
}

void test_live_cleanup(CuTest *tc)
{
    aos_pool_t *p = NULL;
    int is_cname = 0;
    aos_string_t bucket;
    aos_status_t *s = NULL;
    oss_request_options_t *options = NULL;
    aos_table_t *resp_headers = NULL;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, is_cname);

    //delete test bucket
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    s = oss_delete_bucket(options, &bucket, &resp_headers);
    CuAssertIntEquals(tc, 204, s->code);
    CuAssertPtrNotNull(tc, resp_headers);

    aos_pool_destroy(p);
}

void test_create_live_channel_default(CuTest *tc)
{
    aos_pool_t *p = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    aos_list_t publish_url_list;
    aos_list_t play_url_list;
    oss_live_channel_configuration_t *config = NULL;
    oss_live_channel_configuration_t info;
    char *live_channel_id = "test_live_channel_create_def";
    char *live_channel_desc = "my test live channel";
    aos_string_t bucket;
    aos_string_t channel_id;
    oss_live_channel_publish_url_t *publish_url;
    oss_live_channel_play_url_t *play_url;
    char *content = NULL;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    aos_str_set(&channel_id, live_channel_id);

    config = oss_create_live_channel_configuration_content(options->pool);
    aos_str_set(&config->id, live_channel_id);
    aos_str_set(&config->description, live_channel_desc);

    // create
    aos_list_init(&publish_url_list);
    aos_list_init(&play_url_list);
    s = oss_create_live_channel(options, &bucket, config, &publish_url_list,
        &play_url_list, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    aos_list_for_each_entry(publish_url, &publish_url_list, node) {
        content = apr_psprintf(p, "%.*s", publish_url->publish_url.len,
            publish_url->publish_url.data);
        CuAssertStrnEquals(tc, AOS_RTMP_PREFIX, strlen(AOS_RTMP_PREFIX), content);
    }

    aos_list_for_each_entry(play_url, &play_url_list, node) {
        content = apr_psprintf(p, "%.*s", play_url->play_url.len,
            play_url->play_url.data);
        CuAssertStrnEquals(tc, AOS_HTTP_PREFIX, strlen(AOS_HTTP_PREFIX), content);
    }

    // get info
    s = oss_get_live_channel_info(options, &bucket, &channel_id, &info, NULL);
    CuAssertIntEquals(tc, 200, s->code);
    content = apr_psprintf(p, "%.*s", info.id.len, info.id.data);
    CuAssertStrEquals(tc, live_channel_id, content);
    content = apr_psprintf(p, "%.*s", info.description.len, info.description.data);
    CuAssertStrEquals(tc, live_channel_desc, content);
    content = apr_psprintf(p, "%.*s", info.status.len, info.status.data);
    CuAssertStrEquals(tc, "enabled", content);
    content = apr_psprintf(p, "%.*s", info.target.type.len, info.target.type.data);
    CuAssertStrEquals(tc, "HLS", content);
    content = apr_psprintf(p, "%.*s", info.target.play_list_name.len, info.target.play_list_name.data);
    CuAssertStrEquals(tc, "play.m3u8", content);
    CuAssertIntEquals(tc, 5, info.target.frag_duration);
    CuAssertIntEquals(tc, 3, info.target.frag_count);

    // delete
    s = oss_delete_live_channel(options, &bucket, &channel_id, NULL);
    CuAssertIntEquals(tc, 204, s->code);

    aos_pool_destroy(p);

    printf("test_create_live_channel_default ok\n");
}

void test_create_live_channel(CuTest *tc)
{
    aos_pool_t *p = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    aos_list_t publish_url_list;
    aos_list_t play_url_list;
    oss_live_channel_configuration_t *config = NULL;
    oss_live_channel_configuration_t info;
    char *live_channel_id = "test_live_channel_create";
    char *live_channel_desc = "my test live channel,<>{}=?//&";
    aos_string_t bucket;
    aos_string_t channel_id;
    oss_live_channel_publish_url_t *publish_url;
    oss_live_channel_play_url_t *play_url;
    aos_table_t *resp_headers = NULL;
    char *content = NULL;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    aos_str_set(&channel_id, live_channel_id);

    config = oss_create_live_channel_configuration_content(options->pool);
    aos_str_set(&config->id, live_channel_id);
    aos_str_set(&config->description, live_channel_desc);
    aos_str_set(&config->status, LIVE_CHANNEL_STATUS_DISABLED);
    aos_str_set(&config->target.type, "HLS");
    aos_str_set(&config->target.play_list_name, "myplay.m3u8");
    config->target.frag_duration = 100;
    config->target.frag_count = 99;

    // create
    aos_list_init(&publish_url_list);
    aos_list_init(&play_url_list);
    s = oss_create_live_channel(options, &bucket, config, &publish_url_list,
        &play_url_list, &resp_headers);
    CuAssertIntEquals(tc, 200, s->code);
    CuAssertPtrNotNull(tc, resp_headers);

    aos_list_for_each_entry(publish_url, &publish_url_list, node) {
        content = apr_psprintf(p, "%.*s", publish_url->publish_url.len,
            publish_url->publish_url.data);
        CuAssertStrnEquals(tc, AOS_RTMP_PREFIX, strlen(AOS_RTMP_PREFIX), content);
    }

    aos_list_for_each_entry(play_url, &publish_url_list, node) {
        content = apr_psprintf(p, "%.*s", play_url->play_url.len,
            play_url->play_url.data);
        CuAssertStrnEquals(tc, AOS_RTMP_PREFIX, strlen(AOS_RTMP_PREFIX), content);
    }

    // get info
    s = oss_get_live_channel_info(options, &bucket, &channel_id, &info, NULL);
    CuAssertIntEquals(tc, 200, s->code);
    content = apr_psprintf(p, "%.*s", info.id.len, info.id.data);
    CuAssertStrEquals(tc, live_channel_id, content);
    content = apr_psprintf(p, "%.*s", info.description.len, info.description.data);
    CuAssertStrEquals(tc, live_channel_desc, content);
    content = apr_psprintf(p, "%.*s", info.status.len, info.status.data);
    CuAssertStrEquals(tc, LIVE_CHANNEL_STATUS_DISABLED, content);
    content = apr_psprintf(p, "%.*s", info.target.type.len, info.target.type.data);
    CuAssertStrEquals(tc, "HLS", content);
    content = apr_psprintf(p, "%.*s", info.target.play_list_name.len, info.target.play_list_name.data);
    CuAssertStrEquals(tc, "myplay.m3u8", content);
    CuAssertIntEquals(tc, 100, info.target.frag_duration);
    CuAssertIntEquals(tc, 99, info.target.frag_count);

    // delete
    s = oss_delete_live_channel(options, &bucket, &channel_id, NULL);
    CuAssertIntEquals(tc, 204, s->code);

    aos_pool_destroy(p);

    printf("test_create_live_channel ok\n");
}

void test_get_live_channel_stat(CuTest *tc)
{
    aos_pool_t *p = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    aos_list_t publish_url_list;
    aos_list_t play_url_list;
    oss_live_channel_configuration_t *config = NULL;
    oss_live_channel_stat_t live_stat;
    char *live_channel_id = "test_live_channel_stat";
    char *live_channel_desc = "my test live channel";
    aos_string_t bucket;
    aos_string_t channel_id;
    char *content = NULL;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    aos_str_set(&channel_id, live_channel_id);

    config = oss_create_live_channel_configuration_content(options->pool);
    aos_str_set(&config->id, live_channel_id);
    aos_str_set(&config->description, live_channel_desc);

    // create
    aos_list_init(&publish_url_list);
    aos_list_init(&play_url_list);
    s = oss_create_live_channel(options, &bucket, config, &publish_url_list,
        &play_url_list, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    // get stat
    s = oss_get_live_channel_stat(options, &bucket, &channel_id, &live_stat, NULL);
    CuAssertIntEquals(tc, 200, s->code);
    content = apr_psprintf(p, "%.*s", live_stat.status.len, live_stat.status.data);
    CuAssertStrEquals(tc, "Idle", content);

    s = oss_delete_live_channel(options, &bucket, &channel_id, NULL);
    CuAssertIntEquals(tc, 204, s->code);

    aos_pool_destroy(p);

    printf("test_get_live_channel_stat ok\n");
}

void test_delete_live_channel(CuTest *tc)
{
    aos_pool_t *p = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    aos_list_t publish_url_list;
    aos_list_t play_url_list;
    oss_live_channel_configuration_t *config = NULL;
    oss_live_channel_stat_t live_stat;
    char *live_channel_id = "test_live_channel_del";
    char *live_channel_desc = "my test live channel";
    aos_string_t bucket;
    aos_string_t channel_id;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    aos_str_set(&channel_id, live_channel_id);

    config = oss_create_live_channel_configuration_content(options->pool);
    aos_str_set(&config->id, live_channel_id);
    aos_str_set(&config->description, live_channel_desc);

    // create
    aos_list_init(&publish_url_list);
    aos_list_init(&play_url_list);
    s = oss_create_live_channel(options, &bucket, config, &publish_url_list,
        &play_url_list, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    // get stat
    s = oss_get_live_channel_stat(options, &bucket, &channel_id, &live_stat, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    // delete
    s = oss_delete_live_channel(options, &bucket, &channel_id, NULL);
    CuAssertIntEquals(tc, 204, s->code);

    // get stat
    s = oss_get_live_channel_stat(options, &bucket, &channel_id, &live_stat, NULL);
    CuAssertIntEquals(tc, 404, s->code);
    CuAssertStrEquals(tc, "NoSuchLiveChannel", s->error_code);

    aos_pool_destroy(p);

    printf("test_delete_live_channel ok\n");
}

void test_list_live_channel(CuTest *tc)
{
    aos_pool_t *p = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    aos_string_t bucket;
    oss_list_live_channel_params_t *params = NULL;
    oss_live_channel_content_t *live_chan;
    char *content = NULL;
    oss_live_channel_publish_url_t *publish_url;
    oss_live_channel_play_url_t *play_url;
    int channel_count = 0;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);

    // create
    s = create_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list1");
    CuAssertIntEquals(tc, 200, s->code);
    s = create_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list2");
    CuAssertIntEquals(tc, 200, s->code);
    s = create_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list3");
    CuAssertIntEquals(tc, 200, s->code);
    s = create_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list4");
    CuAssertIntEquals(tc, 200, s->code);
    s = create_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list5");
    CuAssertIntEquals(tc, 200, s->code);

    // list
    params = oss_create_list_live_channel_params(options->pool);
    s = oss_list_live_channel(options, &bucket, params, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    channel_count = 0;
    aos_list_for_each_entry(live_chan, &params->live_channel_list, node) {
        channel_count++;
        content = apr_psprintf(p, "%.*s", live_chan->id.len, live_chan->id.data);
        CuAssertStrEquals(tc, apr_psprintf(p, "test_live_channel_list%d", channel_count), content);
        content = apr_psprintf(p, "%.*s", live_chan->description.len, live_chan->description.data);
        CuAssertStrEquals(tc, "live channel description", content);
        content = apr_psprintf(p, "%.*s", live_chan->status.len, live_chan->status.data);
        CuAssertStrEquals(tc, LIVE_CHANNEL_STATUS_ENABLED, content);
        content = apr_psprintf(p, "%.*s", live_chan->last_modified.len, live_chan->last_modified.data);
        CuAssertStrnEquals(tc, "2016", strlen("2016"), content);
        aos_list_for_each_entry(publish_url, &live_chan->publish_url_list, node) {
            content = apr_psprintf(p, "%.*s", publish_url->publish_url.len, publish_url->publish_url.data);
            CuAssertStrnEquals(tc, AOS_RTMP_PREFIX, strlen(AOS_RTMP_PREFIX), content);
        }
        aos_list_for_each_entry(play_url, &live_chan->play_url_list, node) {
            content = apr_psprintf(p, "%.*s", play_url->play_url.len, play_url->play_url.data);
            CuAssertStrnEquals(tc, AOS_HTTP_PREFIX, strlen(AOS_HTTP_PREFIX), content);
        }
    }
    CuAssertIntEquals(tc, 5, channel_count);

    // list by prefix
    params = oss_create_list_live_channel_params(options->pool);
    aos_str_set(&params->prefix, "test_live_channel_list2");
    s = oss_list_live_channel(options, &bucket, params, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    channel_count = 0;
    aos_list_for_each_entry(live_chan, &params->live_channel_list, node) {
        channel_count++;
        content = apr_psprintf(p, "%.*s", live_chan->id.len, live_chan->id.data);
        CuAssertStrEquals(tc, "test_live_channel_list2", content);
        content = apr_psprintf(p, "%.*s", live_chan->description.len, live_chan->description.data);
        CuAssertStrEquals(tc, "live channel description", content);
        content = apr_psprintf(p, "%.*s", live_chan->status.len, live_chan->status.data);
        CuAssertStrEquals(tc, LIVE_CHANNEL_STATUS_ENABLED, content);
        content = apr_psprintf(p, "%.*s", live_chan->last_modified.len, live_chan->last_modified.data);
        CuAssertStrnEquals(tc, "2016", strlen("2016"), content);
        aos_list_for_each_entry(publish_url, &live_chan->publish_url_list, node) {
            content = apr_psprintf(p, "%.*s", publish_url->publish_url.len, publish_url->publish_url.data);
            CuAssertStrnEquals(tc, AOS_RTMP_PREFIX, strlen(AOS_RTMP_PREFIX), content);
        }
        aos_list_for_each_entry(play_url, &live_chan->play_url_list, node) {
            content = apr_psprintf(p, "%.*s", play_url->play_url.len, play_url->play_url.data);
            CuAssertStrnEquals(tc, AOS_HTTP_PREFIX, strlen(AOS_HTTP_PREFIX), content);
        }
    }
    CuAssertIntEquals(tc, 1, channel_count);

    // list by mark
    params = oss_create_list_live_channel_params(options->pool);
    aos_str_set(&params->marker, "test_live_channel_list2");
    s = oss_list_live_channel(options, &bucket, params, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    channel_count = 0;
    aos_list_for_each_entry(live_chan, &params->live_channel_list, node) {
       channel_count++;
       content = apr_psprintf(p, "%.*s", live_chan->id.len, live_chan->id.data);
       CuAssertStrEquals(tc, apr_psprintf(p, "test_live_channel_list%d", channel_count + 2), content);
       content = apr_psprintf(p, "%.*s", live_chan->description.len, live_chan->description.data);
       CuAssertStrEquals(tc, "live channel description", content);
       content = apr_psprintf(p, "%.*s", live_chan->status.len, live_chan->status.data);
       CuAssertStrEquals(tc, LIVE_CHANNEL_STATUS_ENABLED, content);
       content = apr_psprintf(p, "%.*s", live_chan->last_modified.len, live_chan->last_modified.data);
       CuAssertStrnEquals(tc, "2016", strlen("2016"), content);
       aos_list_for_each_entry(publish_url, &live_chan->publish_url_list, node) {
           content = apr_psprintf(p, "%.*s", publish_url->publish_url.len,
               publish_url->publish_url.data);
           CuAssertStrnEquals(tc, AOS_RTMP_PREFIX, strlen(AOS_RTMP_PREFIX), content);
       }
       aos_list_for_each_entry(play_url, &live_chan->play_url_list, node) {
           content = apr_psprintf(p, "%.*s", play_url->play_url.len, play_url->play_url.data);
           CuAssertStrnEquals(tc, AOS_HTTP_PREFIX, strlen(AOS_HTTP_PREFIX), content);
       }
    }
    CuAssertIntEquals(tc, 3, channel_count);

    // list by max keys
    params = oss_create_list_live_channel_params(options->pool);
    params->max_keys = 3;
    s = oss_list_live_channel(options, &bucket, params, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    channel_count = 0;
    aos_list_for_each_entry(live_chan, &params->live_channel_list, node) {
        channel_count++;
        content = apr_psprintf(p, "%.*s", live_chan->id.len, live_chan->id.data);
        CuAssertStrEquals(tc, apr_psprintf(p, "test_live_channel_list%d", channel_count), content);
        content = apr_psprintf(p, "%.*s", live_chan->description.len, live_chan->description.data);
        CuAssertStrEquals(tc, "live channel description", content);
        content = apr_psprintf(p, "%.*s", live_chan->status.len, live_chan->status.data);
        CuAssertStrEquals(tc, LIVE_CHANNEL_STATUS_ENABLED, content);
        content = apr_psprintf(p, "%.*s", live_chan->last_modified.len, live_chan->last_modified.data);
        CuAssertStrnEquals(tc, "2016", strlen("2016"), content);
        aos_list_for_each_entry(publish_url, &live_chan->publish_url_list, node) {
            content = apr_psprintf(p, "%.*s", publish_url->publish_url.len, publish_url->publish_url.data);
            CuAssertStrnEquals(tc, AOS_RTMP_PREFIX, strlen(AOS_RTMP_PREFIX), content);
        }
        aos_list_for_each_entry(play_url, &live_chan->play_url_list, node) {
            content = apr_psprintf(p, "%.*s", play_url->play_url.len, play_url->play_url.data);
            CuAssertStrnEquals(tc, AOS_HTTP_PREFIX, strlen(AOS_HTTP_PREFIX), content);
        }
    }
    CuAssertIntEquals(tc, 3, channel_count);

    s = delete_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list1");
    CuAssertIntEquals(tc, 204, s->code);
    s = delete_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list2");
    CuAssertIntEquals(tc, 204, s->code);
    s = delete_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list3");
    CuAssertIntEquals(tc, 204, s->code);
    s = delete_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list4");
    CuAssertIntEquals(tc, 204, s->code);
    s = delete_test_live_channel(options, TEST_BUCKET_NAME, "test_live_channel_list5");
    CuAssertIntEquals(tc, 204, s->code);

    aos_pool_destroy(p);

    printf("test_list_live_channel ok\n");
}

void test_get_live_channel_history(CuTest *tc)
{
    aos_pool_t *p = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    aos_list_t live_record_list;
    oss_live_record_content_t *live_record;
    char *live_channel_id = "test_live_channel_hist";
    aos_string_t bucket;
    aos_string_t channel_id;
    aos_string_t content;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    aos_str_set(&channel_id, live_channel_id);

    // create
    s = create_test_live_channel(options, TEST_BUCKET_NAME, live_channel_id);
    CuAssertIntEquals(tc, 200, s->code);

    // get history
    aos_list_init(&live_record_list);
    s = oss_get_live_channel_history(options, &bucket, &channel_id, &live_record_list, NULL);
    CuAssertIntEquals(tc, 200, s->code);

    aos_list_for_each_entry(live_record, &live_record_list, node) {
        aos_str_set(&content, ".000Z");
        CuAssertIntEquals(tc, 1, aos_ends_with(&live_record->start_time, &content));
        CuAssertIntEquals(tc, 1, aos_ends_with(&live_record->end_time, &content));
        CuAssertTrue(tc, live_record->remote_addr.len >= strlen("0.0.0.0:0"));
    }

    // delete
    s = oss_delete_live_channel(options, &bucket, &channel_id, NULL);
    CuAssertIntEquals(tc, 204, s->code);

    aos_pool_destroy(p);

    printf("test_get_live_channel_history ok\n");
}

/**
 * Note: the case need put rtmp stream, use command:
 *  ffmpeg \-re \-i allstar.flv \-c copy \-f flv "rtmp://oss-live-channel.demo-oss-cn-shenzhen.aliyuncs.com/live/test_live_channel_vod?playlistName=play.m3u8"
 */
void test_post_vod_play_list(CuTest *tc)
{
    aos_pool_t *p = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    char *live_channel_id = "test_live_channel_vod";
    aos_string_t bucket;
    aos_string_t channel_id;
    aos_string_t play_list_name;
    aos_string_t start_time;
    aos_string_t end_time;
    oss_acl_e oss_acl;
    aos_table_t *resp_headers = NULL;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    aos_str_set(&channel_id, live_channel_id);

    oss_acl = OSS_ACL_PUBLIC_READ_WRITE;
    s = oss_put_bucket_acl(options, &bucket, oss_acl, &resp_headers);

    // create
    s = create_test_live_channel(options, TEST_BUCKET_NAME, live_channel_id);
    CuAssertIntEquals(tc, 200, s->code);

    // post vod
    aos_str_set(&play_list_name, "play.m3u8");
    apr_time_t now = apr_time_now() / 1000000;
    aos_str_set(&start_time, apr_psprintf(options->pool, "%" APR_INT64_T_FMT, now - 3600));
    aos_str_set(&end_time, apr_psprintf(options->pool, "%" APR_INT64_T_FMT, now + 3600));

    s = oss_post_vod_play_list(options, &bucket, &channel_id, &play_list_name,
        &start_time, &end_time, NULL);
    CuAssertIntEquals(tc, 400, s->code);

    // delete
    s = oss_delete_live_channel(options, &bucket, &channel_id, NULL);
    CuAssertIntEquals(tc, 204, s->code);

    aos_pool_destroy(p);

    printf("test_post_vod_play_list ok\n");
}

void test_gen_rtmp_signed_url(CuTest *tc)
{
    aos_pool_t *p = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    char *live_channel_id = "test_live_channel_url";
    aos_string_t bucket;
    aos_string_t channel_id;
    aos_string_t play_list_name;
    oss_acl_e oss_acl;
    aos_table_t *resp_headers = NULL;
    char *rtmp_url = NULL;
    int64_t expires = 0;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    init_test_request_options(options, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    aos_str_set(&channel_id, live_channel_id);

    oss_acl = OSS_ACL_PRIVATE;
    s = oss_put_bucket_acl(options, &bucket, oss_acl, &resp_headers);

    // create
    s = create_test_live_channel(options, TEST_BUCKET_NAME, live_channel_id);
    CuAssertIntEquals(tc, 200, s->code);

    // gen url
    aos_str_set(&play_list_name, "play.m3u8");
    expires = apr_time_now() / 1000000 + 60 * 30;
    rtmp_url = oss_gen_rtmp_signed_url(options, &bucket, &channel_id,
        &play_list_name, expires, NULL);

    // manual check passed
    CuAssertStrnEquals(tc, AOS_RTMP_PREFIX, strlen(AOS_RTMP_PREFIX), rtmp_url);

    // delete
    s = oss_delete_live_channel(options, &bucket, &channel_id, NULL);
    CuAssertIntEquals(tc, 204, s->code);

    aos_pool_destroy(p);

    printf("test_gen_rtmp_signed_url ok\n");
}

CuSuite *test_oss_live()
{
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, test_live_setup);
    SUITE_ADD_TEST(suite, test_create_live_channel_default);
    SUITE_ADD_TEST(suite, test_create_live_channel);
    SUITE_ADD_TEST(suite, test_get_live_channel_stat);
    SUITE_ADD_TEST(suite, test_delete_live_channel);
    SUITE_ADD_TEST(suite, test_list_live_channel);
    SUITE_ADD_TEST(suite, test_get_live_channel_history);
    SUITE_ADD_TEST(suite, test_post_vod_play_list);
    SUITE_ADD_TEST(suite, test_gen_rtmp_signed_url);
    SUITE_ADD_TEST(suite, test_live_cleanup);

    return suite;
}
