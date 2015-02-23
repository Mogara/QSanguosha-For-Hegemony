--技能讲解5：基本技能类型的组合和复杂技能的实现（一）
--[[
	在游戏中，对于有些技能来说，我们不可能仅仅通过写一个技能来实现这些效果。
	因此，基本技能的组合对于国战(也包括身份)来说是非常重要的。
	在这个文档中，我们会介绍两种技能组合的形式。
	第一种是并列式，也就是两个技能是并列存在的。
	先来看这样一个技能：
]]
--[[
	奇心：锁定技，你的手牌上限增加X，X为你当前的体力；你的攻击范围增加X，X为你损失的体力。
]]
--[[
	分析：通过技能描述，我们可以看出这个技能是由两个基本技能组成：
	1.手牌上限
	2.攻击范围
	因此，我们需要写两个技能就能完成任务。
	代码如下：
]]
devQixin = sgs.CreateMaxCardsSkill{
	name = "devQixin",
	extra_func = function(self, target)
		if target:hasSkill(self:objectName()) and (target:hasShownSkill(self) or target:askForSkillInvoke(self:objectName())) then
			--别忘了，手牌上限技在国战里面可是在服务器执行的，所以可以询问发动技能。
			return target:getHp()
		end
	end
}
devQixinAttackRange = sgs.CreateAttackRangeSkill{
	name = "#devQixin_attack", --技能名以#开头的技能会隐藏起来（没有按钮），但是仍然有效果
	extra_func = function(self, target)
		if target:hasSkill(self:objectName()) and target:hasShownSkill(self) then
			return target:getLostHp()
		end
	end
}
sgs.insertRelatedSkills(extension,"devQixin","#devQixin_attack") 
--最后一句是把两个小技能组合起来，其中extension参数是扩展包，第二个参数是主技能（即不隐藏的技能），后面的是附加技能。
--附加技能可以有很多，如果有很多，可以这样写：
sgs.insertRelatedSkills(extension,"main","#related1","#related2","#related3") 
--这里的字符串也可以换成技能对象，比如说：
sgs.insertRelatedSkills(extension,devQixin,devQixinAttackRange)
--[[
	看完上面的技能，你是否对技能组合有了一定的初步认识？
	这些在身份局的相关教程也被介绍过。
	我们再来看另外一种组合方式：嵌入式
	对于嵌入式技能来说，一般是一个视为技+一个触发技的组合，二者用触发技的view_as_skill成员连接。
	有些情况下，还需要技能卡的参与。
	来看这样一个技能：
]]

--[[
	奸雄：出牌阶段限一次，你可以将一张牌当作本出牌阶段上次使用的牌使用。（杀需要考虑限制）
	其实就是英雄杀的曹操的技能。
]]
--[[
	我们可以把这个技能分成两个小部分：
	1.在使用完基本牌后，记录一下基本牌的id
	2.在视为技中获取id，然后制造出这样一张牌。
]]
devJiaoxiongVS = sgs.CreateViewAsSkill{ --这里可以换成OneCardViewAsSkill
	name = "devJiaoxiong", --这里的name要和下面触发技的保持一致。
	n = 1,
	enabled_at_play = function(self, player)
		if player:getMark(self:objectName()) >= 0  then
			return not player:hasFlag("hasUsed"..self:objectName())
		end
	end,
	view_filter = function(self, selected, to_select)
		return not to_select:isEquipped() 
	end,
	view_as = function(self, cards)
		if #cards == 1 then
			local acard = sgs.Sanguosha:cloneCard(sgs.Sanguosha:getCard(sgs.Self:getMark(self:objectName())):objectName())
			--sgs.Self也可以获取Mark，前提是使用Room的setPlayerMark设置。
			assert(acard) --assert函数将会在后面的章节介绍
			acard:addSubcard(cards[1]:getId())
			acard:setSkillName(self:objectName())
			acard:setShowSkill(self:objectName())
			return acard
		end
	end,
}

devJiaoxiong = sgs.CreateTriggerSkill{
	name = "devJiaoxiong",
	view_as_skill = devJiaoxiongVS,
	events = {sgs.CardFinished,sgs.EventPhaseStart}, 
	can_trigger = function(self, event, room, player, data)
		if player and player:isAlive() and player:hasSkill(self:objectName()) then
			if player:getPhase() ~= sgs.Player_Play then return "" end
			if event == sgs.CardFinished then
				local use = data:toCardUse()
				local card = use.card
				if card:getSkillName() ~= self:objectName() and (not card:isKindOf("EquipCard")) then
					room:setPlayerMark(player,self:objectName(),card:getId()) --这里通过Mark来传递ID
				elseif card:getSkillName() == self:objectName() then
					room:setPlayerFlag(player,"hasUsed"..self:objectName())
					room:setPlayerMark(player,self:objectName(),-1) --貌似有Id为0的Card，所以使用-1
				end
			else
				room:setPlayerMark(player,self:objectName(),-1)
			end
		end
		return ""
	end
}
--[[
	这个技能由一个触发技和一个视为技组成，之间用view_as_skill连接起来。
	其实这种方式和并列式差不多，当然这种方式比并列式更简洁，尤其是在处理提示使用（askForUseCard）的时候相当有效。
	再来看一个相对复杂的技能：
]]
--[[
	箭矢：每当你的黑色牌因弃置而失去时，你可将这些牌置于你的武将牌上称为“箭”。回合外你可将两张“箭”当成【无懈可击】使用。
]]
--[[
	分析：通过技能描述，我们可以看出这个技能是由两个基本技能组成的，一个是将被弃置的黑色牌置于牌堆上，另一种则是视为无懈可击。
	这个技能有新旧两种形式，为了便于教学，我们这里使用旧式的技能作为范例。（新式的技能将会在后面用到）
	代码如下：
]]

devJianshiCard = sgs.CreateSkillCard{
	name = "devJianshiCard", 
	target_fixed = true,
	will_throw = false,
	skill_name = "devJianshi",
	on_validate = function(self, carduse) --这两个函数到下面再解释。
		local source = cardUse.from
		local room = source:getRoom()
		local ncard = sgs.Sanguosha:cloneCard("nullification")
		ncard:setSkillName("devJianshi")
		local ids = source:getPile("devJianshi")
		for i = 0, 1, 1 do
			room:fillAG(ids, source)
			local id = room:askForAG(source, ids, false, "devJianshi")
			ncard:addSubcard(id)
			ids:removeOne(id)
			room:clearAG(source)
		end
		return ncard
	end,
	on_validate_in_response = function(self, source) --同理
		local room = source:getRoom()
		local ncard = sgs.Sanguosha:cloneCard("nullification")
		ncard:setSkillName("devJianshi")
		local ids = source:getPile("devJianshi")
		for i = 0,1,1 do
			room:fillAG(ids, source)
			local id = room:askForAG(source, ids, false, "devJianshi")
			ncard:addSubcard(id)
			ids:removeOne(id)
			room:clearAG(source)
		end
		return ncard
	end,
}
devJianshiVS = sgs.CreateZeroCardViewAsSkill{ --详细定义可以参考lua\sgs_ex.lua
	name = "devJianshi",
	view_as = function(self) 
		local ac = devJianshiCard:clone()
		ac:setSkillName("devJianshi")
		ac:setShowSkill("devJianshi")
		return ac
	end, 
	enabled_at_play = function(self, player)
		return false
	end,
	enabled_at_response = function(self, player, pattern)
		return pattern == "nullification" and player:getPile("devJianshi"):length() > 1
	end,
	enabled_at_nullification = function(self, player)
		return player:getPile("devJianshi"):length() > 1 
	end
}
devJianshi = sgs.CreateTriggerSkill{
	name = "devJianshi",
	view_as_skill = devJianshiVS,
	events = {sgs.BeforeCardsMove},
	can_trigger = function(self, event, room, player, data)
		if player and player:isAlive() and player:hasSkill(self:objectName()) then
			local move = data:toMoveOneTime()
			if move.from and move.from:objectName() ~= player:objectName() then return "" end
			if bit32.band(move.reason.m_reason, sgs.CardMoveReason_S_MASK_BASIC_REASON) == sgs.CardMoveReason_S_REASON_DISCARD then
				for i = 0, move.card_ids:length()-1, 1 do
					local id = move.card_ids:at(i)
					card = sgs.Sanguosha:getCard(id)
					if move.from_places:at(i) == sgs.Player_PlaceHand or move.from_places:at(i) == sgs.Player_PlaceEquip then
						if card:isBlack() then							
							return self:objectName()
						end
					end					
				end
			end
			return ""
		end
	end,
	on_cost = function(self, event, room, player, data)
		if player:askForSkillInvoke(self:objectName(), data) then
			return true 
		end
		return false
	end ,
	on_effect = function(self, event, room, player, data)
		local move = data:toMoveOneTime()
		local card
		local dummy = sgs.Sanguosha:cloneCard("jink") 
		for i = 0, move.card_ids:length()-1, 1 do
			local id = move.card_ids:at(i)
			card = sgs.Sanguosha:getCard(id)
			if move.from_places:at(i) == sgs.Player_PlaceHand or move.from_places:at(i) == sgs.Player_PlaceEquip then
				if card:isBlack() then							
					dummy:addSubcard(id)
				end
			end					
		end
		for _,id in sgs.qlist(dummy:getSubcards()) do
			move.card_ids:removeOne(id)
		end
		data:setValue(move) --如果对data做过更变一定不要忘记setValue
		player:addToPile("devJianshi", dummy:getSubcards())
		dummy:deleteLater() --记住，DummyCard不用一定要删除，否则会造成内存泄漏。
	end,
}
--[[
	仔细研究一下这个技能，也有很多值得学习的地方，这里我们就不深入研究了。
	
	通过以上几个例子发现，当一个技能在我们面前的时候，我们可以按照这样的步骤去完成技能代码：
	1.分析技能，如果技能用一个基本技能就可以解决，就用一个技能解决；否则的话考虑使用两个或多个技能
	2.分清职责，什么技能处理什么效果一定要明白。必要时可以使用注视。
	3.撰写代码，这一步没有什么说的
	4.组合，千万别忘记sgs.insertRelatedSkills或者view_as_skill
]]