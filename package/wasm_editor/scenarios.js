const WAITING_TIME_MAX = 2000;

// Testing functions from editor.html:
//   testing_visible(id)        - Returns true if element is visible
//   testing_value(id)          - Returns element value: '1'/'0' for checkbox, "'a' 'b'" for list and node tree, false if element is missing
//   testing_action(id, value)  - Clicks element with onclick, otherwise sets value and dispatches 'change'. Returns false if element is missing
//   testing_scene_stat()       - Returns scene counters: { arrowsCount, lineSetsCount, boundingSpheresCount, boundingBoxesCount, voxelMeshesCount, groundMeshesCount, vegetationsCount, particleEmittersCount }
//   testing_mtool_position()   - Returns moving tool position { x, y, z }, fields are undefined if there is no moving tool

async function expect(page, fn, expected, name) {
    const where = new Error().stack.split('\n')[3].trim();
    await page.waitForFunction(`(${fn})() === ${expected}`, null, { timeout: WAITING_TIME_MAX }).catch(e => {
        throw new Error(name + ' failed: ' + where + '\n' + e.message);
    });
}
function EXPECT_TRUE(page, fn) {
    return expect(page, fn, true, 'EXPECT_TRUE');
}
function EXPECT_FALSE(page, fn) {
    return expect(page, fn, false, 'EXPECT_FALSE');
}
function EXPECT_VISIBLE(page, id) {
    return expect(page, `() => testing_visible('${id}')`, true, 'EXPECT_VISIBLE');
}
function EXPECT_HIDDEN(page, id) {
    return expect(page, `() => testing_visible('${id}')`, false, 'EXPECT_HIDDEN');
}
async function ACTION(page, id, value) {
    const where = new Error().stack.split('\n')[2].trim();
    const clickable = await page.evaluate(id => document.getElementById(id)?.onclick != null, id);
    const done = clickable
        ? page.click('#' + id, { timeout: WAITING_TIME_MAX }).then(() => true)
        : page.evaluate(([id, value]) => testing_action(id, value), [id, value]);
    if (await done.catch(e => { throw new Error('ACTION failed: ' + id + ' ' + where + '\n' + e.message); }) == false) {
        throw new Error('ACTION failed: ' + id + ' ' + where);
    }
}

module.exports = {
    scenario_node_menu_is_opened: async page => {
        await EXPECT_VISIBLE(page, 'button_node_new');
        await ACTION(page, 'button_node_new');
        await EXPECT_VISIBLE(page, 'add_node_menu');
        await ACTION(page, 'node_tree');
        await EXPECT_HIDDEN(page, 'add_node_menu');
    },
    scenario_voxel_mesh_is_moved: async page => {
        await ACTION(page, 'button_node_new');
        await EXPECT_VISIBLE(page, 'add_node_menu');
        await ACTION(page, 'new_node_10');
        await EXPECT_HIDDEN(page, 'add_node_menu');
        await EXPECT_VISIBLE(page, 'node_0');
        await EXPECT_VISIBLE(page, 'node_1_resource_list');
        await EXPECT_TRUE(page, () => document.getElementById('node_resource_list').options.length > 0);
        const model = await page.evaluate(() => document.getElementById('node_resource_list').options[0].value);
        await ACTION(page, 'node_resource_list', model);
        await EXPECT_TRUE(page, () => testing_scene_stat().voxelMeshesCount == 1);
        await ACTION(page, 'inspect_node_x', '5');
        await EXPECT_TRUE(page, () => testing_mtool_position().x == 5);
    },
};
